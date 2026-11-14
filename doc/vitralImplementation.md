# Convergencia con VITRAL: implementación

povCpp se construye sobre la biblioteca `vitral_base` del checkout de VITRAL
(`VITRAL_CPP_ROOT`, por defecto `~/VITRAL/vitral/cpp`). VITRAL no depende de
povCpp. La copia local `base/src/main` es una copia de referencia idéntica a
VITRAL (`scripts/auditVitralConvergence.py`); los headers y la biblioteca del
build proceden del checkout externo.

## Modelo compartido

```text
VITRAL base
  Geometry (todas las geometrías)         [VITRAL; PovRay no define ninguna]
  SceneSolid <- SimpleBody <- CsgBody     [VITRAL scene]
  SceneSolid <- PovrayBody <- CsgOperand  [PovRay src/environment]
  CsgTree (CsgBodyOperand)                [CSG de VITRAL y de PovRay]
  CsgExpression <- ConstructiveSolidGeometry [PovRay src/environment/scene]
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
de operandos son los del árbol. El CSG no es una geometría: lo posee su cuerpo
(`PovrayBody::getCsg`, `setCsg`), que lo traza, prueba contención, copia,
invierte y acota a través de él. Un CSG anidado es una hoja (`CsgOperand`) que
posee otro `ConstructiveSolidGeometry`; su vista en el árbol
(`CsgOperandBodyView`) lo expone como expresión hija. El evaluador lo elige el parser con el
`CsgBodyEvaluator` de VITRAL: `CROSSING_MEMBERSHIP` usa `CsgMorganRulesEvaluator`
(la heurística de PovRay 1.0) y `ROTH` (`-csgRoth`) usa `CsgRaySegmentEvaluator`.
Los evaluadores rápidos del render (la mezcla de
Roth y el filtro de cruces de la heurística) usan las plantillas de VITRAL
`CsgSegmentClassification` y `CsgCrossingSelection` sobre los eventos de PovRay,
tanto en los evaluadores POV como en el bake.

## Código de render de PovRay

`src/environment/geometry` contiene el trazado POV de las geometrías VITRAL:
las colas de cruces y el estado por rayo (`RayWithTracingState`,
`IntersectionCandidate`, `PriorityQueuePool`), los códigos de contención
(`RayCastingContainment`), los kernels por familia (`trace`, con
`PatchScratch` del parche) y el detalle de superficie (`SurfaceDetailKernel`).
Los volúmenes de acotación del render son el paquete VITRAL
`environment/geometry/boundingVolumeHierarchy` (`PreRayHitElement`,
`BoundingVolumeHierarchy`, `AabbBoundingVolume`, `AxisAlignedBoundingBox`,
`RaySlabReciprocals`, `AabbBinHierarchy`); `RayWithTracingState` guarda los
recíprocos de la dirección de cada rayo para la prueba de placas, y los bins
de operandos de una unión ancha (`OperandCullBins`) son un `AabbBinHierarchy`. `src/environment/scene` contiene
`PovrayBody`, los composites de instancias (`RayCastingComposite`), el CSG POV
(`constructiveSolidGeometry`), el recorrido proyectado de triángulos
(`ProjectedTriangleTraversal`), el orden invertido de cruces de un cuerpo
invertido (`BoxInvertedCrossingSink`) y el paso de cruces VITRAL a la cola POV
(`NativeGeometryCrossingSink`). PovRay no define subclases de `Geometry`
(`scripts/auditVitralConvergence.py` lo comprueba).

El trazado POV de una superficie VITRAL se elige por su familia
(`SurfaceDetailFamily`) en `SurfaceTraceKernel`: emisión de cruces, primer
impacto y contención con el estado POV de la superficie (`SurfaceTraceState`:
inversión, término cuadrado no nulo, caché por punto de vista de los rayos
primarios y recorrido proyectado de mallas) y las tolerancias históricas de
PovRay. Ese estado pertenece al cuerpo: `PovrayBody` (y por tanto
`CsgOperand`) guarda la superficie VITRAL, su familia y su
`SurfaceTraceState`, y traza con el kernel de la familia; los registros del
bake leen el estado del cuerpo. La inversión de caja, polinomio, blob, esfera
y geometría genérica es el indicador del estado
(`SurfaceTraceKernel::invertsByState`); la cuádrica y el plano la codifican en
sus coeficientes, la malla en su recorrido proyectado, el CSG POV por De
Morgan, y PovRay la ignora en campos de altura, parches y luces. Las familias son `QuadricTrace`, `BoxTrace`, `PlaneTrace`,
`PolynomialTrace`, `BlobTrace`, `HeightFieldTrace`, `BicubicPatchTrace`
(`src/environment/geometry/trace`), `NativeSurfaceTrace` (esferas y geometrías
genéricas) y `ProjectedTriangleTraversal` (mallas); la geometría de luces es la
familia vacía. Sus núcleos numéricos son de VITRAL: `QuadricRayExpansion` y
`QuadricSurface::solve`, `Box::intersectBounds`,
`InfinitePlane::intersectionDepth`, `SphereTrace`, `BlobSolid::intersect`,
`PolynomialSurface::intersect`, `HeightFieldSolid::traceFirstHit` y
`SubdividedBezierPatch::intersect`. La contención de campos de altura conserva
la prueba truncada a enteros de PovRay 1.0. Los registros del bake guardan
superficie, familia y estado al construirse y llaman directamente al kernel
de la familia; solo un CSG POV trazado como objeto genérico usa su propio
evaluador. El parser construye directamente las geometrías VITRAL
(`PlanarHalfSpace`, `Box`, `Sphere`, `TriangleMesh`, `QuadricSurface`,
`PolynomialSurface`, `BlobSolid`, `HeightFieldSolid`, `SubdividedBezierPatch`)
y `PovrayBody` y `SimpleBodyBuilder` guardan un `Geometry`. El cuerpo elige
por familia el volumen envolvente (`SurfaceBoundingVolume`), la copia, la
inversión de coeficientes y el estado inicial (`SurfaceTraceKernel`); una malla
de triángulos POV es un `TriangleMesh` con el `ProjectedTriangleTraversal` que
posee el cuerpo. Las reglas de imagen de los campos de altura (borde y nivel de
agua, alturas fraccionarias POT y TGA) y los tipos de subdivisión del parche
son del parser. Una `light_source` dentro de un objeto es un cuerpo que posee
la luz (`PovrayBody::getLight`, familia vacía: sin cruces ni interior); el
enlace de luces entrega a la escena una copia colocada.

Un impacto se identifica por la superficie VITRAL en la que cae y su familia
(`IntersectionAttributes::getHitSurface` y `SurfaceDetailFamily`), por el
cuerpo dueño y por la cadena de dueños de detalle (cuerpos, operandos CSG y
composites, que implementan `PostRayHitElement`). `SurfaceDetailKernel`
calcula la normal según la familia con las reglas de normalización y de normal
degenerada de PovRay; los adaptadores no intervienen en el detalle. Los
registros del bake obtienen la superficie de su almacenamiento horneado
(un `QuadricSurface` o un `PlanarHalfSpace`) o de su cuerpo.

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
repite el nombre de una clase de VITRAL ni deriva de una geometría de VITRAL
(las subclases transitivas de `Geometry`), que el inventario
(`doc/vitralConvergenceInventory.json`, hoy vacío) solo nombra archivos
existentes y que la copia local de `base/src/main` es idéntica a VITRAL.

Desde `/home/jedilink/VITRAL/vitral/cpp`:

```bash
cmake -S . -B build -DWITH_JPEG=ON \
  -DVITRAL_VGA_PORTABLE_SOURCE_DIR=/home/jedilink/VITRAL/vitral/cpp/x86_64_smp_kernel/src/main
cmake --build build -j12
ctest --test-dir build --output-on-failure
```

## Correspondencia con la literatura

Las APIs públicas de VITRAL que usa la convergencia y el algoritmo publicado
que implementan. Las etiquetas son las de los comentarios del código; donde el
código nombra el método sin etiqueta se da la referencia completa.

| API de VITRAL | Algoritmo | Referencia |
|---|---|---|
| `CsgRothRayClassifier`, `CsgSegmentClassification`, `CsgMembershipRule`, `CsgBodyEvaluator::ROTH` | Clasificación de rayos sobre el árbol CSG: segmentos IN/OUT por hoja combinados por las tablas de las operaciones (§2.2, §3.3, tabla 3) | [ROTH1982] Roth, S. D. "Ray Casting for Modeling Solids", *Computer Graphics and Image Processing* 18(2), 1982 |
| `CsgCrossingMembershipFilter`, `CsgBodyEvaluator::CROSSING_MEMBERSHIP` | Heurística CSG de POV-Ray 1.0: cada cruce de una hoja se conserva según la pertenencia del punto a los operandos hermanos | Código fuente de POV-Ray 1.0 (`doc/references/originalPovray1Authors.md`) |
| `AxisAlignedBoundingBox::intersectsRayForward`, `intersectsRayBefore`, `RaySlabReciprocals` | Prueba de placas rayo/caja con recíprocos de la dirección | Kay, T. L., Kajiya, J. T. "Ray Tracing Complex Scenes", *Computer Graphics* (SIGGRAPH '86) 20(4), 1986 |
| `AabbBinHierarchy` | Jerarquía de dos niveles: bins de primitivas consecutivas por centroide sobre el eje de mayor dispersión | Sin referencia publicada; equivale a una estructura de nivel inferior de cajas procedurales de Vulkan (`VK_GEOMETRY_TYPE_AABBS_KHR`) |
| `Triangle` (consulta rayo/triángulo) | Intersección rayo/triángulo sin almacenamiento adicional | Möller, T., Trumbore, B. "Fast, Minimum Storage Ray-Triangle Intersection", *Journal of Graphics Tools* 2(1), 1997 |
| `SubdividedBezierPatch`, `ParametricBiCubicPatch`, `_ParametricBiCubicPatchIntersector` | Parche bicúbico: red de control de Bézier, subdivisión de de Casteljau y árbol de esferas envolventes; inversa de la matriz de base de Bézier (11.28) | [WAYN1990] Knapp, W. "Ray with Bicubic Patch Intersection Problem", *Ray Tracing News* 3(3), 1990; [FOLE1992] Foley, van Dam, Feiner, Hughes, *Computer Graphics: Principles and Practice*, 2.ª ed., 1992; [POVR1993] `bezier.c` de POV-Ray 1.0/2.x |
| `PolynomialSurface`, `BlobSolid` | Raíces reales sobre el rayo por secuencias de Sturm (o la fórmula cerrada de la cuártica) | El código no cita una referencia |
| `PolyhedralBoundedSolid`, `PolyhedralBoundedSolidEulerConstructions`, `Cone` | Representación por semiaristas y operadores de Euler (`mvfs`, `smev`, `smef`, `lmev`, `lmef`); lámina circular y barrido traslacional (12.2, 12.3.1) | [MANT1988] Mäntylä, M. *An Introduction to Solid Modeling*, Computer Science Press, 1988 |
| `_PolyhedralBoundedSolidSetOperator` y clasificadores de `polyhedralBoundedSolidOperators/booleans` | Operaciones booleanas de 2-variedades por clasificación de vecindades de vértices | [MANT1986] Mäntylä, M. "Boolean Operations of 2-Manifolds through Vertex Neighborhood Classification", *ACM Transactions on Graphics* 5(1), 1986; [MANT1988] |
| `MonotoneDecompositionTriangulator` | Descomposición trapezoidal incremental aleatorizada y triangulación de polígonos | [SEID1991] Seidel, R. "A Simple and Fast Incremental Randomized Algorithm for Computing Trapezoidal Decompositions and for Triangulating Polygons", *Computational Geometry: Theory and Applications* 1(1), 1991 |
| `VoxelVolume` | Conversión a vóxeles y recorrido incremental de la grilla | [KAUF1987] Kaufman, A., SIGGRAPH '87; [SNYD1987] Snyder, J., Barr, A., SIGGRAPH '87; [AMAN1987] Amanatides, J., Woo, A., Eurographics '87 |
| `ProceduralNoise` | Ruido de gradiente y turbulencia; hipertextura | [PERL1985] Perlin, K. "An Image Synthesizer", SIGGRAPH '85; [PERL1989] Perlin, K., Hoffert, E. "Hypertexture", SIGGRAPH '89 |
| `AffineEllipsoidMinimumDistance`, `AffineEllipsoidDistanceBounds` | Distancia certificada a esferas bajo transformaciones afines | `doc/vitralCertifiedEllipsoidBounds.md` |
