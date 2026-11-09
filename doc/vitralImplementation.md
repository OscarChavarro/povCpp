# Convergencia con VITRAL: implementación y verificación

Actualización de alcance: `raycasting` se ha trasladado a `./raycasting` en
povCpp. VITRAL se construye sin esta biblioteca; la convergencia de sus paquetes
`geometry` y `scene` con `base` queda para otro plan. Los resultados y el traslado
a VITRAL descritos abajo corresponden a las etapas anteriores. Los contratos de
cruces ahora están separados por clase y `SurfaceDetail` no usa namespaces.
La copia local se ha ampliado a 144 archivos idénticos; véase `vitralBaseSync.json`.
Validación de este ajuste: VITRAL compila sin raycasting (109 tests habilitados),
povCpp pasa 110 tests y `renderAll.sh` produce las mismas 108 imágenes en 112,12 s.
La escena offline `balls.ray` sigue siendo idéntica a la referencia.

Trabajo realizado el 2026-11-08/09, sin commits, sobre:

- povCpp: `3d29446211c486bdc514cd21f91f60cb20a74fb4`.
- VITRAL: `e8ecd78d5bc298cc02189f53cb9d01ef122c3e7c`.

Los cambios de ambos árboles son una entrega conjunta: el nuevo povCpp necesita
el checkout VITRAL con estas modificaciones, no solamente su revisión anterior.
`doc/bvhPlan.md`, que ya estaba sin seguimiento, no se ha modificado.

## Modelo compartido implementado

```text
VITRAL base
  Geometry <- RayCastingGeometry        [VITRAL raycasting]
  SimpleBody <- RayCastingBody          [VITRAL raycasting]
  SimpleScene <- Scene                  [aplicación PovRay]
  SimpleSceneSnapshot / Material / CameraSnapshot / Light

ReaderMitScene -> SimpleScene -> SimpleRaytracer / ParallelRaytracer
Parser .pov   -> Scene        -> RenderEngine y shaders POV
```

La colección de cuerpos y luces de `Scene` es la de `SimpleScene`. Los snapshots
referencian esos mismos objetos. La geometría de `RayCastingBody` tiene un único
propietario, su base `SimpleBody`; no se conserva una copia paralela del grafo.
`bakedScene` continúa siendo una representación derivada y específica del
renderer POV.

Se ha creado `VITRAL/vitral/cpp/raycasting`, con el paquete público
`vsdk/toolkit/environment/raycasting`. Allí están las primitivas, ambas estrategias
CSG, operandos, bounds, cuerpos, composites, candidatos y recursos de consulta que
antes estaban en `src/environment/geometry` y parte de `src/environment/scene`.
Las implementaciones trasladadas se eliminan de povCpp. La copia local de `base/src/main` se ha restaurado posteriormente a petición del
usuario: sus 135 archivos son idénticos a VITRAL. Es una copia de referencia;
headers y biblioteca del build proceden del checkout externo.

Se mantienen locales `PovRayMaterial`, pigmentos, evaluadores de normales,
shaders, parser/builders `.pov`, configuración POV y la extensión `Scene`
(niebla, IOR atmosférico, dimensiones, antialias y vida de la textura por defecto).
La biblioteca VITRAL no importa headers de la aplicación.

Se mantienen los nombres de las primitivas originales de VITRAL. Las variantes
que necesitan la semántica histórica de cruces son `RayCastingSphere`,
`RayCastingBox`, `RayCastingInfinitePlane`, `RayCastingTriangleMesh`, etc. Son
extensiones de la misma jerarquía, no clases globales incompatibles que colisionan
al enlazar. El cálculo de raíces de esfera está disponible en VITRAL y lo usa
la variante de ray casting.

## Contratos y compatibilidad

- `SurfaceDetail` define las máscaras comunes.
- `Geometry` y `SimpleBody` exponen la capacidad opcional `emitRayCrossings`;
  una operación no soportada se distingue de cero impactos.
- Los eventos incluyen parámetro, geometría hoja, cuerpo y material cuando están
  disponibles. El adaptador de los kernels históricos declara su orientación
  `Unclassified`; no inventa estados entrada/salida.
- Las consultas comunes de primer impacto y detalle recuperan la atribución CSG
  y sus propietarios; la ruta nativa conserva sus pools y caches por trabajador.
- `getSurfaceMaterial()` expone `Material*`; la proyección antigua a
  `SimpleMaterial*` rechaza familias incompatibles, sin reinterpretarlas.
- Los cuerpos afines publican la matriz combinada, posición y capacidad de
  edición. La rotación/escala TRS se rechaza cuando no representa su colocación;
  las operaciones nativas conservan el orden histórico y notifican modificaciones.
- El límite de ocho propietarios de detalle ahora produce un error explícito
  cuando se excede; antes se descartaban niveles silenciosamente.
- Los artefactos de VITRAL usados por povCpp se generan dentro de su propio
  build. No sobrescriben la biblioteca que usan los ejemplos de VITRAL.
- `renderAll.sh` guarda cada PID al lanzarlo y devuelve error si falla alguno.
  Se preservan las 108 escenas, la resolución y el paralelismo del script.

## Construcción reproducible

Desde povCpp:

```bash
./scripts/compile.sh \
  -DVITRAL_CPP_ROOT=/home/jedilink/VITRAL/vitral/cpp \
  -DVITRAL_BUILD_TESTS=OFF
ctest --test-dir build --output-on-failure
./scripts/renderAll.sh
```

Desde `/home/jedilink/VITRAL/vitral/cpp`:

```bash
cmake -S . -B build -DWITH_JPEG=ON \
  -DVITRAL_VGA_PORTABLE_SOURCE_DIR=/home/jedilink/VITRAL/vitral/cpp/x86_64_smp_kernel/src/main
cmake --build build -j12
ctest --test-dir build --output-on-failure
```

El build original omitía VGA por no tener configurada esa ruta. Al habilitarlo
se detectó y corrigió el orden de includes: las cabeceras de los ejemplos deben
preceder a las de los programas del kernel. Compilan `vitral_vga`,
`vitral_rasterizer` y `vitral_raytracer`, además del resto de testsuite. No se han
modificado fuentes del kernel ni se ha probado una sesión gráfica VGA.

## Verificación

Los registros completos de esta sesión están en `/tmp/vitral-convergence-baseline`,
`/tmp/vitral-convergence-after` y los logs `/tmp/vitral-*.log` / `/tmp/pov-*.log`.
El resumen persistente y hashes finales se guardan en `vitralValidation.json`.

- Referencia inicial: 108 imágenes, antes de sustituir biblioteca o geometrías.
- Primer enlace con VITRAL: imágenes idénticas; 118,97 s frente a 126,51 s.
- Repetición emparejada: referencia 128,09 s; integración 117,45 s. CPU de usuario:
  1407,92 s frente a 1403,78 s.
- Traslado a biblioteca estática: imágenes idénticas, 112,14 s.
- Consulta común y plantillas independientes: imágenes idénticas, 110,11 s.
- Gate final sobre la última versión: 110.74 s; las 108 imágenes siguen idénticas.
- Build VITRAL con todos los ejemplos habilitados: correcto.
- Tests VITRAL: 110 habilitados pasan; un test de referencia booleana ya estaba
  deshabilitado y sigue así. Incluyen CSG compartido y render secuencial/paralelo
  de un cuerpo del nuevo módulo con el renderer VITRAL existente.
- Test de povCpp: verifica identidad del grafo, geometría consultada como
  `Geometry`, cuerpo consultado como `SimpleBody`, colocación, snapshot y reset.
- Las 12 escenas MIT se renderizan en secuencial y paralelo, iguales byte a byte.
  `balls.ray` también coincide con la imagen anterior a la migración.

Los tiempos se obtienen con `/usr/bin/time -v`, sin compilaciones ni otros renders
propios concurrentes. No constituyen una garantía para cualquier equipo o escena;
son evidencia del gate concreto solicitado. No se regeneraron golden images.

## Normalización que permanece separada

La convergencia funcional del environment está aplicada. No se afirma que todas
las normalizaciones propuestas en el plan estén terminadas:

- Los kernels originales de primer impacto y los de ray casting conservan sus
  tolerancias y comportamiento distintos. Fusionarlos todos requiere otra
  validación numérica; moverlos a VITRAL no demuestra su equivalencia.
- La representación TRS y las dos capas afines conviven como estrategias de
  colocación del modelo común. No se fuerza una descomposición que pierda shear.
- El contrato de bounds histórico se adapta a `getMinMax`; no se han reemplazado
  todos los consumidores VITRAL por el nuevo modelo de volúmenes.
- UV/tangente siguen en los pipelines existentes; el adaptador común suministra
  punto y normal. No se presenta como implementación completa de esos detalles.
- Los parámetros de render POV siguen en su extensión `Scene`, sin introducir
  todavía un `RenderRequest` independiente.

Estas diferencias son explícitas y no necesitan dos modelos editables de escena
ni mantener geometrías privadas dentro de povCpp. El próximo trabajo de
normalización puede hacerse dentro de VITRAL, con ambos consumidores ya enlazados
al mismo modelo y los gates conservados.
