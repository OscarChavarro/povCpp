# Convergencia con VITRAL: implementación

povCpp se construye sobre la biblioteca `vitral_base` del checkout de VITRAL
(`VITRAL_CPP_ROOT`, por defecto `~/VITRAL/vitral/cpp`). VITRAL no depende de
povCpp. La copia local `base/src/main` es una copia de referencia idéntica a
VITRAL (`scripts/auditVitralConvergence.py`); los headers y la biblioteca del
build proceden del checkout externo.

## Modelo compartido

```text
VITRAL base
  Geometry <- RayCastingGeometry          [PovRay src/environment/geometry]
  SceneSolid <- SimpleBody <- CsgBody     [VITRAL scene]
  SceneSolid <- PovrayBody <- CsgOperand  [PovRay src/environment]
  CsgTree (CsgBodyOperand)                [CSG de VITRAL y de PovRay]
  SimpleScene <- Scene                    [PovRay src/environment/scene]
  SimpleSceneSnapshot / Material / CameraSnapshot / Light

ReaderMitScene -> SimpleScene -> SimpleRaytracer / ParallelRaytracer
Parser .pov   -> Scene        -> bake POV (bakedScene) -> RenderEngine y shaders POV
```

`SceneSolid` es el contrato común de cuerpos de escena: referencia de geometría,
colocación afín autoritativa y las consultas tipadas calculadas una sola vez a
partir del sólido denotado (`queryDenotedSolid`). `SimpleBody` añade edición TRS
y apariencia VITRAL; `PovrayBody` añade las dos capas de colocación de PovRay,
su apariencia, la inversión y las regiones `bounded_by`/`clipped_by`. `Scene`
posee sus `PovrayBody` en una lista propia; de `SimpleScene` usa luces y
recursos. Los renderers de VITRAL dibujan cuerpos `SimpleBody`. `bakedScene` es
la representación derivada y específica del renderer POV.

El CSG de PovRay es un `CsgTree` de VITRAL: `ConstructiveSolidGeometry` posee
sus hojas `CsgOperand` (un `PovrayBody` con una sola capa de colocación y el
material del operando) y las ve como `CsgBodyOperand`; la operación y el orden
de operandos son los del árbol. Un CSG anidado es una hoja cuya geometría es otro
`ConstructiveSolidGeometry`. Los evaluadores rápidos del render (la mezcla de
Roth y el filtro de cruces de la heurística) usan las plantillas de VITRAL
`CsgSegmentClassification` y `CsgCrossingSelection` sobre los eventos de PovRay,
tanto en los evaluadores POV como en el bake.

## Código de render de PovRay

`src/environment/geometry` contiene la interfaz de render POV de las
geometrías (`RayCastingGeometry`), sus colas de cruces y estado por rayo
(`RayWithTracingState`, `IntersectionCandidate`, `PriorityQueuePool`), los
volúmenes de acotación del render (`AxisAlignedBoundingBox`, que transforma
sus esquinas con `GeometryBounds`) y los evaluadores CSG POV con `CsgOperand`.
`src/environment/scene` contiene `PovrayBody`, los composites de instancias
(`RayCastingComposite`) y el enlace de render de esferas, mallas y otras
geometrías VITRAL (`NativeGeometryBinding`).

El trazado POV de una superficie VITRAL se elige por su familia
(`SurfaceDetailFamily`) en `SurfaceTraceKernel`: emisión de cruces, primer
impacto y contención con el estado POV de la superficie (`SurfaceTraceState`:
inversión, término cuadrado no nulo, caché por punto de vista de los rayos
primarios y recorrido proyectado de mallas) y las tolerancias históricas de
PovRay. Las familias son `QuadricTrace`, `BoxTrace`, `PlaneTrace`,
`PolynomialTrace`, `BlobTrace`, `HeightFieldTrace`, `BicubicPatchTrace`
(`src/environment/geometry/trace`), `NativeSurfaceTrace` (esferas y geometrías
genéricas) y `ProjectedTriangleTraversal` (mallas); la geometría de luces es la
familia vacía. Sus núcleos numéricos son de VITRAL: `QuadricRayExpansion` y
`QuadricSurface::solve`, `Box::intersectBounds`,
`InfinitePlane::intersectionDepth`, `SphereTrace`, `BlobSolid::intersect`,
`PolynomialSurface::intersect`, `HeightFieldSolid::traceFirstHit` y
`SubdividedBezierPatch::intersect`. La contención de campos de altura conserva
la prueba truncada a enteros de PovRay 1.0. Los registros del bake guardan
superficie, familia y estado al construirse y llaman al kernel sin pasar por
el adaptador; solo un CSG POV trazado como objeto genérico usa su propio
evaluador. Los adaptadores (`Quadric`, `RayCastingBox`,
`RayCastingInfinitePlane`, `PolynomialShape`, `Blob`, `HeightField`,
`RayCastingParametricBiCubicPatch`, `LightGeometryAdapter`) contienen la
geometría VITRAL y su estado, y delegan en los mismos kernels.

Un impacto se identifica por la superficie VITRAL en la que cae y su familia
(`IntersectionAttributes::getHitSurface` y `SurfaceDetailFamily`), por el
cuerpo dueño y por la cadena de dueños de detalle (cuerpos, operandos CSG y
composites, que implementan `PostRayHitElement`). `SurfaceDetailKernel`
calcula la normal según la familia con las reglas de normalización y de normal
degenerada de PovRay; los adaptadores no intervienen en el detalle. Los
registros del bake obtienen la superficie de su almacenamiento horneado
(cuádrica o plano) o del adaptador original.

Materiales, pigmentos, normales, shaders, parser `.pov`, configuración y la
extensión `Scene` (niebla, IOR atmosférico, dimensiones, antialias y vida de la
textura por defecto) son de PovRay.

## Consultas y colocación

- `doIntersectionFirstHit` y `doExtraInformation` de `SceneSolid` sobre un
  `PovrayBody` usan la ruta nativa de cruces y detalle con un workspace
  temporal (`RayCastingQueryAdapter`); dan punto y normal, no UV ni tangente.
- La ruta nativa reutiliza `RayWithTracingState` y el pool de colas del
  llamador.
- `emitRayCrossings` distingue una capacidad no soportada de un rayo sin
  impactos. Los eventos se ordenan por parámetro y conservan hoja, cuerpo y
  material. Los eventos de la ruta nativa son `Kind::Unclassified`; la
  pertenencia sólida se obtiene de las operaciones CSG o de contención.
- El detalle de un impacto transformado o CSG se resuelve con la cadena de
  propietarios de detalle, limitada a ocho niveles
  (`IntersectionAttributes::MAX_DETAIL_OWNERS`); exceder el límite lanza una
  excepción.
- `PovrayBody` conserva dos capas afines y su orden de construcción;
  `getAffinePlacement()` da la colocación combinada. Se edita con `translate`,
  `rotate` y `scale`.
- `getSurfaceMaterial()` expone `Material*`. `SimpleBody::getMaterial()` da
  `SimpleMaterial*` y rechaza otras familias de materiales.
- Los snapshots referencian elementos de la escena: la escena debe seguir viva y
  sin cambios hasta terminar el render. El snapshot posee su cámara.

## Construcción y verificación

Desde povCpp:

```bash
./scripts/compile.sh -DVITRAL_CPP_ROOT=/home/jedilink/VITRAL/vitral/cpp
ctest --test-dir build --output-on-failure
./scripts/renderAll.sh
python3 scripts/auditVitralConvergence.py
```

`povray_environment_tests` y `povray_geometry_tests` comprueban el puente entre
el entorno compartido y el de PovRay: cruces CSG, atribución de hoja, material y
cuerpo, desborde de la cadena de detalle y render serial/paralelo con los
raytracers de VITRAL. La auditoría comprueba que no existe el módulo
`raycasting`, que VITRAL no incluye código de PovRay, que ninguna clase de PovRay
repite el nombre de una clase de VITRAL, que las entradas pendientes del
inventario (`doc/vitralConvergenceInventory.json`) existen y que la copia local
de `base/src/main` es idéntica a VITRAL.

Desde `/home/jedilink/VITRAL/vitral/cpp`:

```bash
cmake -S . -B build -DWITH_JPEG=ON \
  -DVITRAL_VGA_PORTABLE_SOURCE_DIR=/home/jedilink/VITRAL/vitral/cpp/x86_64_smp_kernel/src/main
cmake --build build -j12
ctest --test-dir build --output-on-failure
```
