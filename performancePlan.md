# Plan de recuperación de rendimiento

## Contexto

Entre el commit `8c3cee6b` y HEAD se introdujo una regresión de ~79% en el tiempo de render de `iortest.pov` (59s → 106s). Todos los golden tests pasan, por lo que el comportamiento lógico es correcto; solo la eficiencia fue degradada por una serie de refactors.

Herramienta: gprof (`-pg -O3`) sobre ambos commits ejecutando `iortest.pov +w1280 +h800`.

---

## Análisis de los perfiles

### Comparativa de los hotspots principales

| Función                              | HEAD (gprof-s) | OLD (gprof-s) | Delta      |
|--------------------------------------|---------------|---------------|------------|
| `Box::intersectBoxx`                 | 2.68 s        | n/a (0 s)     | **+2.68 s**|
| `Box::allBoxIntersections`           | 0.56 s        | 1.22 s        | -0.66 s    |
| `PriorityQueuePool::pqPop`           | 0.97 s        | n/a (0 s)     | **+0.97 s**|
| `Transformation::MInverseTransformVector` | 0 s      | 0.48 s        | —          |
| `Transformation::MInvTransVector`    | 0 s           | 0.39 s        | —          |
| `InfinitePlane::allPlaneIntersections`| 2.21 s       | 2.12 s        | +0.09 s    |
| `Sphere::allSphereIntersections`     | 1.66 s        | 1.44 s        | +0.22 s    |
| `Statistics::global()`               | 1.23 s        | 1.13 s        | +0.10 s    |

Las dos regresiones dominantes son los dos primeros deltas en la tabla.

---

## Causa raíz 1 (principal): temporales Matrix4x4d en el hot path de intersección

### Qué pasó

La migración `Transformation` → `Matrix4x4d` (commits `7004675`, `ffb51e2`) reemplazó:

```cpp
// ANTIGUO: aritmética directa sobre arrays, ~32 bytes de stack
Transformation::MInverseTransformVector(&p, &ray->position, box->Transform);
Transformation::MInvTransVector(&d, &ray->direction, box->Transform);
```

por:

```cpp
// NUEVO: cadena inmutable de métodos, crea 9 matrices temporales por llamada
p = box->transformationInverse->transpose().multiply(ray->position);
d = box->transformationInverse->transpose().withoutTranslation().multiply(ray->direction);
```

### Desglose del costo por llamada

La segunda línea (`d = ...`) genera 9 objetos `Matrix4x4d` temporales en el stack:

1. `transpose()` → 1 copia completa (16 doubles × 8 bytes = **128 bytes**)
2. `withoutTranslation()` → llama `withVal` 7 veces encadenadas, cada una crea una copia completa → **7 × 128 = 896 bytes**
3. Total por intersección: **1 024 bytes** de stack para `d` + **128 bytes** para `p`

Con 179M tests de Box: **~207 GB de datos temporales escritos y leídos en el stack**. Esto destruye la caché L1/L2 y explica por qué la regresión real (47s) es mayor de lo que gprof indica (que solo mide ciclos, no cache-miss cost).

Adicionalmente, `Matrix4x4d::multiply(Vector3Dd)` realiza una división homogénea `1/rw` donde `rw` es siempre ≈ 1.0 para matrices de transformación afines (translate/rotate/scale), por lo que la división nunca cambia el resultado pero consume ciclos.

### Shapes afectados (paths de intersección en el inner loop)

| Archivo                                  | Líneas con el patrón | Calls/frame aprox. |
|------------------------------------------|----------------------|--------------------|
| `src/environment/geometry/volume/Box.cpp` | 77–79 (`intersectBoxx`) | 179M |
| `src/environment/geometry/volume/polynomial/PolynomialShape.cpp` | 70–72 | alto |
| `src/environment/geometry/volume/Blob.cpp` | 331–332 | moderado |
| `src/environment/geometry/volume/HeightField.cpp` | 776–778 | moderado |

Las funciones `insideBox`, `boxNormal` y equivalentes en otros shapes también usan el patrón pero con frecuencia mucho menor.

### Fix 1a: Agregar métodos inline `transformPoint` / `transformDirection` a Matrix4x4d

Agregar en `Matrix4x4d.h` como funciones `inline` (para que el compilador las inline en los call sites):

```cpp
// Equivale a transpose().multiply(v) sin crear ningún temporal.
// result[i] = sum_k m_[k][i] * v[k] + m_[3][i]
inline Vector3Dd transformPoint(const Vector3Dd& v) const {
    double vx = v.x(), vy = v.y(), vz = v.z();
    return Vector3Dd(
        m_[0][0]*vx + m_[1][0]*vy + m_[2][0]*vz + m_[3][0],
        m_[0][1]*vx + m_[1][1]*vy + m_[2][1]*vz + m_[3][1],
        m_[0][2]*vx + m_[1][2]*vy + m_[2][2]*vz + m_[3][2]
    );
}

// Equivale a transpose().withoutTranslation().multiply(v) sin crear ningún temporal.
// result[i] = sum_k m_[k][i] * v[k]   (sin traslación)
inline Vector3Dd transformDirection(const Vector3Dd& v) const {
    double vx = v.x(), vy = v.y(), vz = v.z();
    return Vector3Dd(
        m_[0][0]*vx + m_[1][0]*vy + m_[2][0]*vz,
        m_[0][1]*vx + m_[1][1]*vy + m_[2][1]*vz,
        m_[0][2]*vx + m_[1][2]*vy + m_[2][2]*vz
    );
}
```

Derivación matemática: `(M^T * [v,1])_i = sum_k M[k][i]*v[k] + M[3][i]` que, en índices row-major
(`m_[row][col]`), es exactamente lo de arriba. La forma `transformDirection` omite la columna de
traslación, equivalente a `withoutTranslation()` antes de multiplicar. El `rw` siempre es 1 para
matrices afines, así que no se necesita la división.

### Fix 1b: Reemplazar el patrón en todos los hot paths

En cada archivo afectado, sustituir:

```cpp
// ANTES
foo = mat->transpose().multiply(v);
bar = mat->transpose().withoutTranslation().multiply(d);

// DESPUÉS
foo = mat->transformPoint(v);
bar = mat->transformDirection(d);
```

Archivos a modificar en el hot path de intersección:
- `Box.cpp` líneas 77–79 (+ lines 219, 247 para insideBox/boxNormal si se desea)
- `PolynomialShape.cpp` líneas 70–72 (+ 117, 787, 810 para completitud)
- `Blob.cpp` líneas 331–332 (+ 435, 466, 492)
- `HeightField.cpp` líneas 776–778 (+ 863, 917, 938–939)

---

## Causa raíz 2 (secundaria): `withoutTranslation()` crea 7 copias por llamada

### Qué pasó

```cpp
// Matrix4x4d.cpp línea 55
Matrix4x4d Matrix4x4d::withoutTranslation() const {
    return withVal(0,3,0.0).withVal(1,3,0.0).withVal(2,3,0.0)
           .withVal(3,0,0.0).withVal(3,1,0.0).withVal(3,2,0.0).withVal(3,3,1.0);
}
```

Cada `withVal` llama al copy-constructor de Matrix4x4d (copia 16 doubles). La cadena de 7 produce
7 objetos temporales de 128 bytes cuando el efecto puede lograrse con 1 copia y 7 asignaciones
directas.

### Fix 2: Reimplementar `withoutTranslation()` directamente

En `Matrix4x4d.cpp`:

```cpp
Matrix4x4d Matrix4x4d::withoutTranslation() const {
    Matrix4x4d r(*this);
    r.m_[0][3] = r.m_[1][3] = r.m_[2][3] = 0.0;
    r.m_[3][0] = r.m_[3][1] = r.m_[3][2] = 0.0;
    r.m_[3][3] = 1.0;
    return r;
}
```

Este fix beneficia cualquier llamada a `withoutTranslation()` fuera del hot path (normales,
setup de escena). Para el hot path de intersección, Fix 1 ya elimina la llamada por completo.

---

## Causa raíz 3 (terciaria): `PriorityQueuePool::pqPop` no se inlinea

### Qué pasó

gprof muestra `PriorityQueuePool<Intersection>::pqPop(int)` a 4.62% (0.97s) con 436M llamadas en HEAD,
ausente del perfil de OLD. En OLD, `pqPop` terminaba con `Logger::error() + exit(1)` en el branch
de error, lo que le indicaba al compilador que el branch no retorna y facilitaba inlinear la función.
Tras el commit `db09ed4 Logger revisited`, se cambió a `Logger::reportMessage()` (que sí retorna),
bloqueando el inlining.

El cuerpo de `pqPop` es trivial (lectura/escritura de un puntero), pero sin inlining paga la
overhead de function-call 436M veces.

### Fix 3: Mover `pqPop` al header para forzar inlining

En `PriorityQueuePool.h` agregar el cuerpo `inline`:

```cpp
template <class T>
inline PriorityQueue<T>*
PriorityQueuePool<T>::pqPop(int indexSize) {
    static constexpr int MAX_NUMBER_OF_ENTRIES = 128;
    if (indexSize >= MAX_NUMBER_OF_ENTRIES) indexSize = MAX_NUMBER_OF_ENTRIES - 1;
    if (head == nullptr) {
        Logger::reportMessage("PriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of prioqs");
    }
    PriorityQueue<T>* pq = head;
    if (pq == nullptr) return nullptr;
    head = pq->next_pq;
    pq->queueSize = indexSize;
    pq->currentEntry = 0;
    return pq;
}
```

(Y eliminar la definición en `PriorityQueuePool.txx`.)

Análogamente para `pqPush`.

---

## Priorización y estimación de impacto

| Fix | Impacto estimado | Esfuerzo | Riesgo |
|-----|-----------------|----------|--------|
| 1a + 1b: `transformPoint`/`transformDirection` | **~35–45s recuperados** (principal) | Medio | Bajo (math verificable) |
| 2: `withoutTranslation()` directa | ~2–5s recuperados | Bajo | Mínimo |
| 3: `pqPop` inline | ~3–8s recuperados | Bajo | Mínimo |

El Fix 1 es el más crítico porque elimina la asignación de 1024 bytes de stack por intersección,
recuperando la presión de caché que es la causa real de los 47s de regresión.

---

## Prerequisitos y criterio de aceptación

- Los golden tests de imagen (todos los niveles) deben seguir pasando byte-idénticos.
- Se verifica ejecutando el gate completo (`renderAll.sh` + `testAgainstGoldenImages.sh`) después de
  cada fix.
- El tiempo de render de `iortest.pov` sin profiling debe regresar a ≤65s (aceptando algo de ruido
  respecto a los 59s originales debidos a diferencias de código irrelevantes al hot path).

---

## Orden de implementación sugerido

1. **Fix 1a**: Agregar `transformPoint`/`transformDirection` a `Matrix4x4d.h` como inline.
   - Verificar que el compilador las inlinea con `-S` o gprof.
2. **Fix 1b**: Reemplazar el patrón `transpose().withoutTranslation().multiply()` en `Box.cpp`
   primero (mayor impacto por call count), ejecutar golden gate.
3. **Fix 1b** (continuación): Migrar `PolynomialShape.cpp`, `Blob.cpp`, `HeightField.cpp`.
4. **Fix 2**: Reimplementar `withoutTranslation()`.
5. **Fix 3**: Inlinear `pqPop`/`pqPush`.
6. Ejecutar gate completo + medir tiempo final de `iortest.pov`.
