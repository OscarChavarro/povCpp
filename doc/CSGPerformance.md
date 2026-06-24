# Plan de mejora de desempeño: `-csgRoth`

Commit analizado: `a7f039c2ee158088e7263050ac34bfdbb0e82f76` ("CSG implementation
for segmented line segments implementing [ROTH1982]").

## 1. Síntoma reportado

`drums.pov` (`etc/level3/drums2`), que en modo por defecto tarda ~91s a
1280×800, tarda **~13 minutos** con `-csgRoth` — ~8.5× más lento — a pesar de
que el resultado visual es equivalente (gate `testCsgRoth.sh` en verde).

## 2. Reproducción y medición

Comandos usados (cwd `etc/level3/drums2`), a 320×200 para acelerar la
iteración:

```sh
../../../build/povray +l../../include +idrums.pov +o/tmp/b.tga +w320 +h200 -d -v +x +ft
../../../build/povray +l../../include +idrums.pov +o/tmp/a.tga +w320 +h200 -d -v +x +ft -csgRoth
```

| Modo | Tiempo real (320×200) | Extrapolado ×16 a 1280×800 | Reportado por el usuario |
|---|---|---|---|
| Por defecto | 5.79 s | 92.6 s | 91 s |
| `-csgRoth` | 49.05 s | 784.8 s (13.1 min) | 13 min |
| **Razón** | **8.48×** | | |

La extrapolación lineal por número de píxeles coincide casi exactamente con
los tiempos reportados en producción, confirmando que el 320×200 es
representativo y que el problema escala linealmente con el número de rayos,
no es un caso patológico de una sola escena a una sola resolución.

Los contadores de `Statistics` (impresos por `-v`) son **prácticamente
idénticos** entre ambos modos para esta escena:

| Contador | Por defecto | `-csgRoth` |
|---|---|---|
| Pruebas de plano | 91 153 576 | 91 209 326 |
| Pruebas de cuádrica | 68 775 936 | 68 816 704 |
| Rayos reflejados | 46 899 | 46 581 |
| Rayos transmitidos | 30 980 | 31 004 |

**Esto es la pista clave**: el volumen de trabajo geométrico bruto (cuántas
veces se prueba un rayo contra una primitiva real) es el mismo. La
diferencia de 8.5× no viene de hacer más pruebas geométricas — viene del
*costo por nodo* de la maquinaria de clasificación booleana de Roth en sí
misma.

## 3. Metodología de profiling

Se compiló una copia instrumentada con `gprof` (`-pg -O2`) en un árbol de
build separado (`/tmp/build_profile`), reutilizando el mismo
`CMakeLists.txt` (que fija `RUNTIME_OUTPUT_DIRECTORY` a `<repo>/build`, así
que hay que tener cuidado de no mezclar binarios entre corridas — el primer
intento de este análisis comparó un `gmon.out` contra el binario equivocado,
ya recompilado sin `-pg`, y produjo atribuciones sin sentido, como funciones
de `CSGByRaySegment` apareciendo en la corrida sin `-csgRoth`. Quedó
corregido reconstruyendo el binario `-pg` y generando+analizando ambos
`gmon.out` antes de tocar el binario de nuevo).

```sh
cmake -DCMAKE_CXX_FLAGS="-pg -O2" -DCMAKE_EXE_LINKER_FLAGS="-pg" -B /tmp/build_profile .
cmake --build /tmp/build_profile --target povray
# (ambas corridas, mismo binario -pg, sin recompilar entre medio)
gprof build/povray gmon_default.out --flat-profile
gprof build/povray gmon_roth.out --flat-profile
```

Nota aparte: dentro del `--flat-profile` de `-csgRoth` aparece
`CSGByRaySegment::invertGeometry()` con 202M+ llamadas — un número
absurdo, dado que esa función solo se invoca al construir el árbol CSG para
`difference` (no por rayo). Se verificó con `nm -C build/povray` que es un
**artefacto de gprof**: el compilador generó clones especializados
(`constprop.0/1/2`) de `mergeByMembership` para cada puntero a función
(`combineUnion`/`combineIntersection`/`combineDifference`), esos clones son
símbolos locales sin entrada propia en la tabla que usa gprof, y caen en el
rango de direcciones justo después de `invertGeometry()`, el símbolo global
más cercano. El tiempo real reportado bajo esa etiqueta (6.7% del total)
pertenece en realidad a `mergeByMembership` y sus tres clones.

## 4. Hallazgos: ¿a dónde se va el tiempo?

Perfil plano de `-csgRoth` (320×200), top de funciones por tiempo propio:

| % | seg. propios | llamadas | función |
|---|---|---|---|
| 39.18% | 9.42 | 6 801 984 570 | `RaySegmentCrossing::RaySegmentCrossing()` *(ctor por defecto)* |
| 10.86% | 2.61 | 220 586 816 | `CSGByRaySegment::buildRaySegments` |
| 9.53% | 2.29 | 353 564 246 | `RaySegments::RaySegments(ArrayList const&, bool)` |
| 6.74% | 1.62 | 202 327 209+ | `mergeByMembership` y clones *(mal etiquetado `invertGeometry`, ver §3)* |
| 4.41% | 1.06 | 68 816 704 | `Quadric::allIntersectionsForMaterial` *(trabajo geométrico real)* |
| 3.74% | 0.90 | 266 899 275 | `PriorityQueuePool<IntersectionCandidate>::push` |
| 3.74% | 0.90 | 132 977 430 | `ArrayList<RaySegmentCrossing>::operator=` |
| 3.45% | 0.83 | 87 609 386 | `CSGByRaySegment::allIntersections` |
| 3.12% | 0.75 | 91 209 326 | `InfinitePlane::allIntersectionsForMaterial` *(trabajo geométrico real)* |
| 2.45% | 0.59 | 156 601 910 | `Quadric::doContainmentTest` |
| 1.04% | 0.25 | 84 311 040 | `RaySegmentCrossing::RaySegmentCrossing(double, bool, IntersectionCandidate const&)` |
| 0.58% | 0.14 | 265 954 860 | `RaySegments::isInitialInside()` |
| 0.50% | 0.12 | 353 564 246 | `RaySegments::getCrossings()` |
| 0.29% | 0.07 | 81 137 194 | `RaySegmentCrossing::isEntering()` |

Comparado con el perfil plano por defecto (mismo binario), donde **no
existe ninguna de las filas de `RaySegments`/`RaySegmentCrossing`** — el
75% superior del tiempo en modo por defecto son las pruebas geométricas
reales (`CSG::allCsgIntersectIntersections` 19.7%, `Quadric::...` 16.2%,
`InfinitePlane::...` 13.7%, etc.).

**Sumando solo las filas que son pura infraestructura de
`RaySegments`/`RaySegmentCrossing`/`ArrayList<RaySegmentCrossing>`** (sin
contar las pruebas geométricas legítimas, que cuestan lo mismo en ambos
modos): **≈72.6% del tiempo total muestreado de `-csgRoth`** se gasta
construyendo, copiando y destruyendo estas estructuras — trabajo que no
existe en absoluto en el algoritmo por defecto.

## 5. Causa raíz

### 5.1 `java::ArrayList<T>` siempre construye por defecto el arreglo completo

```cpp
// base/src/main/java/util/ArrayList.txx
template <class T> void
ArrayList<T>::init() {
    if ( maxSize > 0 ) {
        Data = new T[maxSize];   // construye TODOS los maxSize elementos,
        ...                       // no solo los currentSize que se usarán
    }
}
```

El mismo patrón (`new T[maxSize]` seguido de copiar solo `currentSize`
elementos) se repite en el constructor de copia, `operator=`, `add()` (al
crecer) y `reserve()`. Cada `java::ArrayList<RaySegmentCrossing>{8}`
construido en `buildRaySegments` (220.6M llamadas) **siempre** construye 8
`RaySegmentCrossing` por defecto, aunque para una primitiva típica (un
plano, una cuádrica) el número real de cruces sea 0, 1 o 2.

### 5.2 `RaySegments`/`RaySegmentCrossing` no tienen semántica de movimiento

`ArrayList<T>` declara constructor de copia, `operator=` y destructor
(regla de los 3) pero **no** declara constructor/asignación de movimiento.
Por la regla de C++11, eso **suprime** la generación implícita de
movimiento — toda "transferencia" de un `ArrayList`/`RaySegments` (retorno
por valor sin NRVO, asignación `result = mergeUnion(...)` en el fold de
`CSGByRaySegment::allIntersections`, paso de `RaySegments` por valor) cae
en una copia profunda completa, que vuelve a pagar el costo de §5.1.

### 5.3 Multiplicación por nivel de árbol

`buildRaySegments` se llama 2.52× por cada llamada a
`allIntersections` (fan-out promedio de hijos por nodo CSG en este árbol).
Cada nivel de recursión en el árbol CSG de `drums.pov` (compuestos de
compuestos: `Drum`, `TomTom`, `BassDrum`, `Tensioner`, `Cymbal`, cada uno
con varios `union`/`intersection` anidados) repite el costo de §5.1+§5.2,
por cada rayo (primario, de sombra, reflejado, transmitido). Por eso el
costo no es un overhead fijo pequeño: crece multiplicativamente con la
profundidad/anchura del árbol CSG de la escena, que es exactamente lo que
tiene `drums.pov` y lo que no tiene, por ejemplo, `skyvase.pov` (árbol CSG
plano, de ahí que su render no mostrara este problema de forma notoria).

## 6. Plan de mejora (técnicas estándar de eficiencia algorítmica)

Ordenado por relación impacto/riesgo, de mayor a menor:

### 6.1 Pool de buffers reutilizables para `RaySegmentCrossing` (alto impacto, riesgo medio)

Aplicar el mismo patrón que ya existe para
`PriorityQueuePool<IntersectionCandidate>` (`pop()`/`push()` de buffers
preasignados) a los arreglos de `RaySegmentCrossing` que
`buildRaySegments`/`mergeByMembership` crean en cada llamada. Esto elimina
la asignación/liberación repetida (`new T[8]`/`delete[]`) y, combinado con
6.2, elimina la mayoría de las 6.8 mil millones de construcciones por
defecto. Es el cambio de mayor impacto porque ataca directamente la fila
#1 del perfil (39% del tiempo).

### 6.2 Move semantics en `ArrayList<T>`, `RaySegments` y `RaySegmentCrossing` (alto impacto, bajo riesgo)

Añadir constructor y operador de asignación de movimiento a
`ArrayList<T>` (transferencia de puntero + reseteo del origen, sin
asignar memoria nueva ni construir nada) y dejar que `RaySegments`/
`RaySegmentCrossing` los hereden por generación implícita (al no declarar
ellos mismos destructor/copia personalizados, la regla de los 5 permite que
el compilador genere movimiento automáticamente una vez que `ArrayList<T>`
lo soporte). Beneficia inmediatamente:
- el retorno por valor de `buildRaySegments`/`mergeUnion`/
  `mergeIntersection`/`mergeDifference`,
- la asignación `result = mergeUnion(result, childSegments)` en el fold de
  `allIntersections` (hoy: copia profunda con `new T[8]`; con movimiento:
  intercambio de 3 punteros/enteros).

Cambio acotado, sin alterar ninguna semántica visible — candidato natural
para ir primero y medir.

### 6.3 No construir por defecto los slots no usados de `ArrayList<T>::init()`/`add()`/`reserve()`/copia (impacto alto, riesgo medio-alto)

Es la causa raíz de fondo: `new T[n]` en C++ exige que `T` sea
default-construible y construye los `n` elementos aunque solo se vayan a
usar `currentSize`. Una `ArrayList<T>` que reserve memoria cruda (p. ej.
`::operator new(n * sizeof(T))`) y use *placement new* solo para los
`currentSize` elementos realmente poblados evitaría la sobreconstrucción
(t​ípicamente 5–8 ctores de sobra por cada asignación de 8 elementos, dado
que la mayoría de las primitivas CSG aportan 0–2 cruces reales). Esto es un
cambio más invasivo porque toca el contrato genérico de `ArrayList<T>`
(usado en *todo* el código base, no solo en CSG) y exige revisar con
cuidado el manejo de excepciones/destrucción parcial; **se recomienda
hacerlo después de 6.1/6.2 y medir si sigue siendo necesario** — el pool de
6.1 ya evita la mayoría de las asignaciones, así que el beneficio marginal
de 6.3 podría no justificar el riesgo una vez aplicado 6.1.

### 6.4 Reducir copias intermedias de `RaySegments` en el fold de `allIntersections` (impacto medio, riesgo bajo una vez hecho 6.2)

Una vez con movimiento (6.2), el fold
`result = mergeUnion(result, childSegments)` ya no copia profundamente,
pero `mergeByMembership` sigue construyendo una `RaySegments` *nueva* en
cada llamada. Evaluar fusionar in-place reutilizando el buffer más grande
de los dos operandos en vez de crear siempre un tercer arreglo, ahora que
6.1 da un buffer reciclable al que apuntar.

### 6.5 (Alcance mayor, opcional) Poda por volumen acotador en árboles CSG profundos

`Composite`/`BoundedGeometry` ya hacen una prueba de *bounding* antes de
recorrer el árbol (`doIntersectionFirstHit` + `doContainmentTest`), pero
solo al nivel del objeto compuesto completo (`Drum`, `TomTom`, etc.), no
entre los hijos internos de un `CSGByRaySegment`. Añadir un *bounding box*
por nodo `CSGByRaySegment` permitiría saltar `buildRaySegments` para hijos
que el rayo no puede tocar en absoluto. Mayor alcance, fuera de este plan
inmediato — anotado para una iteración futura si 6.1–6.4 no bastan para
escenas con árboles CSG muy profundos.

## 7. Validación esperada

Después de cada cambio (6.1, luego 6.2, etc.), repetir exactamente:

```sh
cd etc/level3/drums2
time ../../../build/povray +l../../include +idrums.pov +o/tmp/a.tga +w320 +h200 -d -v +x +ft -csgRoth
```

y comparar contra la línea base de este documento (49.05s). Gates
obligatorios sin cambios de comportamiento:

```sh
./scripts/testAgainstGoldenImages.sh   # debe seguir en "Test passed." (bit-idéntico, flag por defecto apagado)
./scripts/testCsgRoth.sh               # debe seguir en verde (mismas 8 fallas preexistentes no relacionadas)
```

**Meta**: reducir el factor 8.5× a algo cercano a 2-3× (el mínimo
estructural esperado por hacer doble pasada de clasificación booleana en
vez de prueba de membresía simple), sin tocar ningún resultado visual.

## 8. Resumen para quien solo lee el final

El 73% del tiempo de `-csgRoth` en escenas con árboles CSG profundos
(`drums.pov`) se gasta en construir, copiar y destruir arreglos pequeños de
`RaySegmentCrossing` — no en pruebas geométricas adicionales (esas cuestan
exactamente lo mismo que en el modo por defecto). La causa es doble: (1)
`java::ArrayList<T>` siempre construye por defecto el arreglo completo en
cada asignación/copia, y (2) ni `ArrayList<T>` ni `RaySegments` tienen
semántica de movimiento, así que cada "transferencia" de estos objetos por
el árbol de recursión cae en una copia profunda. Las dos primeras medidas
del plan (pool reutilizable + move semantics) atacan directamente estas dos
causas y son de bajo riesgo porque no cambian ninguna decisión de
clasificación booleana, solo cómo se gestiona la memoria alrededor de ella.

## 9. Resultado de la ejecución del plan

Implementado y medido incrementalmente, mismo comando de §7, mismo
`drums.pov` 320×200 (línea base 49.05s, modo por defecto sin cambios 5.79s):

| Paso | Cambio | Tiempo `-csgRoth` | Mejora acumulada | Razón vs. por defecto |
|---|---|---|---|---|
| 0 | (línea base, commit `a7f039c`) | 49.05 s | 1.00× | 8.48× |
| 1 | Move semantics en `ArrayList<T>` (constructor/asignación de movimiento; §6.2) | 45.41 s | 1.08× | 7.85× |
| 2 | Accesores y constructores triviales de `RaySegmentCrossing`/`RaySegments` movidos a los headers como `inline` (validación empírica de la pregunta "¿ayuda marcar constructores inline en el hot path?" — ver §9.1) | 40.69 s | 1.21× | 7.03× |
| 3 | `ArrayList<RaySegmentCrossing>` dimensionado exacto en vez de capacidad fija `{8}` (`buildRaySegments`/`mergeByMembership`; versión concreta de §6.3, sin tocar el contrato genérico de `ArrayList<T>`) | 20.22 s | **2.43×** | 3.49× |
| 4 | `RaySegments` recibe el `ArrayList` por valor + `std::move` en cada sitio de retorno, para que el dimensionado del paso 3 se transfiera por movimiento en vez de copiarse de nuevo | **18.8 s** (promedio de 3 corridas: 18.58/18.72/18.99) | **2.61×** | **3.26×** |

**Meta del documento original** (§7): bajar de 8.48× a "algo cercano a
2-3×". Resultado: **3.26×**, prácticamente en el rango, con **2.61× de
mejora absoluta** sobre la implementación original del commit `a7f039c`.

Gates ejecutados después de cada paso, siempre en verde:

```
./scripts/testAgainstGoldenImages.sh   →  "Test passed."  (bit-idéntico, sin cambios)
./scripts/testCsgRoth.sh               →  mismas 8 fallas preexistentes no relacionadas (blob, iortest,
                                            pawns, pool, teapot, wg5, bezier, bezier0), ninguna nueva
```

### 9.1 Nota sobre el paso 2: ¿ayuda `inline` con el build a `-O0`?

El build de producción (`build/CMakeCache.txt`) tiene
`CMAKE_BUILD_TYPE` y `CMAKE_CXX_FLAGS` **vacíos** — compila sin ninguna
bandera de optimización. Esto importa para la pregunta "¿inlinear ayuda?":

- El keyword `inline` por sí solo **no** fuerza inlining — solo relaja la
  regla de definición única (ODR) para permitir múltiples definiciones
  idénticas en distintas unidades de traducción. La decisión real de
  inlinear la toma el optimizador, que en GCC está **desactivado a `-O0`**.
- Más importante: `RaySegmentCrossing.cpp`/`RaySegments.cpp` definían estos
  métodos en su **propia unidad de traducción**. Sin LTO, ningún nivel de
  optimización ni ningún `inline` en el `.cpp` puede hacer que el cuerpo sea
  visible — y por lo tanto inlineable — en el punto de llamada de otra
  unidad (`CSGByRaySegment.cpp`). Había que mover el cuerpo al header
  primero, sin importar el nivel de optimización.

A pesar de que el inliner de GCC está apagado a `-O0`, **el cambio sí
midió una mejora real y reproducible** (45.41s → 40.69s, ~10%, confirmado
en 2 corridas adicionales con 40.62s y 40.63s). La hipótesis más plausible
(no confirmada con un segundo profiler para descartar del todo ruido de
`gprof`): en un ejecutable PIE, una llamada a una función definida en *otra*
unidad de traducción puede resolverse con una indirección adicional
relativa a la GOT incluso dentro del mismo binario final, mientras que una
función visible en el header, definida en la *misma* unidad que la llama,
se resuelve como llamada directa relativa a PC — sin necesidad de que el
optimizador la inlinee de verdad. Esto no se confirmó con certeza absoluta,
pero la mejora medida es consistente y se mantiene con los gates en verde,
así que se conserva.

**Conclusión práctica para el resto del código**: mover funciones triviales
y muy invocadas (getters, constructores pequeños) del `.cpp` al header como
`inline` es una mejora de bajo riesgo que vale la pena incluso en un build
sin optimizaciones — pero el verdadero salto de desempeño en este caso
(paso 3, 2× adicional) vino de dejar de sobre-asignar memoria, no de
inlining. Si se quisiera exprimir más con inlining puro, el camino de mayor
impacto sería habilitar `-O2`/`-O3` para el build de producción (decisión
de alcance mayor al de este documento, no tomada aquí) en vez de perseguir
`inline` función por función.

## 10. Cambios pendientes del plan original

- **6.1 (pool reutilizable de buffers)**: no fue necesario en la forma
  completa descrita originalmente (con `RaySegments` apuntando a un buffer
  prestado en vez de poseer su propio `ArrayList`) — el dimensionado exacto
  del paso 3 combinado con move semantics (pasos 1 y 4) capturó la mayor
  parte de esa ganancia con muchísimo menor riesgo (sin tocar
  `RayWithSegments`/`RenderEngine` ni el modelo de ownership de
  `RaySegments`). Queda como optimización futura si algún escenario más
  extremo lo justifica.
- **6.3 (`ArrayList<T>::init()` sin construir por defecto los slots no
  usados)**: igual que 6.1, su motivación principal (evitar el `new T[8]`
  con 8 elementos sin usar) quedó resuelta por el dimensionado exacto del
  paso 3, que ya no pide más capacidad de la que se va a usar. Sigue siendo
  válida como mejora genérica de `ArrayList<T>` para otros usos del
  contenedor en el resto del código, pero deja de ser urgente para
  `-csgRoth`.
- **6.5 (poda por volumen acotador por nodo)**: no implementada — alcance
  mayor, sin necesidad demostrada todavía dado que 3.26× ya está cerca de
  la meta original.
