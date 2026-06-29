# Performance Review Plan

Comparativa serial de las 108 escenas listadas en `scripts/renderAll.sh`.

- Commit base: `7d9703f5029d2bbdbfb3859a5ba858469ac268e1`
- Commit comparado: `9cff86e4c11499a5e17ab4906cc37befa7ad6eb3`
- Modo de medicion: ejecucion serial, una escena por vez, con los mismos flags de `renderAll.sh`: `+w1280 +h800 -d -v +x +ft` y el include path correspondiente a cada escena.
- Tiempo total commit base: `721.512 s`
- Tiempo total commit comparado: `1518.045 s`
- Factor total: `2.104x`

## Gprof 320x200

Perfilado adicional con `gprof` sobre los mismos dos commits, recompilando ambos worktrees con `-pg` y ejecutando escenas puntuales a `320x200` con los flags `-d -v +x +ft`.

### Resumen

| Escena | 7d9703f (s) | 9cff86e (s) | Factor |
| --- | ---: | ---: | ---: |
| `level2/spline` | 1.651 | 10.418 | 6.311 |
| `level3/ntreal/ntreal` | 1.468 | 6.018 | 4.099 |
| `level3/piece3/piece3` | 4.499 | 14.484 | 3.219 |
| `level2/iortest` | 7.547 | 18.691 | 2.477 |
| `level1/shapes2` | 0.496 | 1.097 | 2.210 |

### Hallazgos transversales

- En `spline`, `ntreal`, `piece3` e `iortest` los contadores de trazado son casi iguales entre commits; la regresion no viene de hacer muchas mas pruebas geometricas.
- El commit `9cff86e` desplaza costo hacia `SimpleBody::doIntersectionForAllRayCrossings`, `CsgOperand::doIntersectionForAllRayCrossings`, clonacion de `RayWithSegments::LocalIntersectionClone`, `PriorityQueuePool`, y construccion de `java::ArrayList<Material*>`.
- Eso es consistente con el modelo desacoplado por matrices: mas trabajo por rayo y por cruce, aunque la escena haga esencialmente el mismo numero de pruebas de interseccion.

### Estado del baking

- En el commit viejo, `Geometry` exponia `translateGeometry/rotateGeometry/scaleGeometry` directamente, o sea que la capa geometry podia mutar sus propios coeficientes y quedar ya transformada al momento del trazado.
- En el commit nuevo, esa API general desaparece de `Geometry`. El desacoplamiento mueve la transformacion al owner (`SimpleBody` y `CsgOperand`) y el trazado entra por matrices inversas aplicadas al rayo y luego reexpresa punto/normal al salir.
- El "baking" no desaparecio del todo: `GeometryTransformMutator` todavia puede mutar `PolynomialShape` y `ParametricBiCubicPatch`.
- Sin embargo, `CsgOperand::translate/rotate/scale` fuerza explicitamente a `PolynomialShape` y `ConstructiveSolidGeometry` por la ruta matricial, y `SimpleBodyBuilder` hace fallback a matrices cuando la geometria no soporta mutacion.
- `PovCameraSpec::bake()` sigue existiendo, pero ese bake es solo de camara y no explica esta regresion.

### Lectura preliminar

- La idea de baking como preprocesamiento geometrico general fue removida de la capa `environment/geometry`; lo que queda es un mutator puntual y no la estrategia dominante del renderer.
- La regresion observada encaja mejor con overhead de la ruta matricial por rayo que con un aumento real del trabajo geometrico de la escena.
- Si se quiere recuperar desempeno sin volver a cargar `Geometry` con estructuras o estado extra, la direccion coherente con el nuevo parametro de diseno es un preprocesamiento en `environment/scene`: por ejemplo, cuerpos/operandos ya normalizados para trazado, caches de transformaciones compuestas, o alguna aceleracion/copia especializada en scene-layer.

### Detalle por escena

#### `level2/spline`

- Estadisticas de render casi identicas.
  - Viejo: `Sphere tests=54,116,360`, `Plane tests=125,362`, `Shadow rays=112,403`
  - Nuevo: `Sphere tests=54,113,350`, `Plane tests=125,395`, `Shadow rays=112,422`
- Hotspots 7d9703f:
  - `Sphere::allIntersectionsForMaterial`: `60.98%`
  - `SimpleBody::allIntersections`: `24.39%`
  - `BoundedGeometry::allIntersections`: `4.88%`
- Hotspots 9cff86e:
  - `CsgOperand::doIntersectionForAllRayCrossings`: `39.81%`
  - `Sphere::doIntersectionForAllRayCrossings`: `19.90%`
  - `RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)`: `10.19%`
  - `PriorityQueuePool::pop/push`: `17.48%` combinado
  - `java::ArrayList<Material*>::init`: `7.28%`
- Lectura:
  - Es el caso mas claro de overhead puro del desacoplamiento. La escena hace casi exactamente el mismo trabajo geometrico, pero ahora paga por operand wrappers, clonacion de rayos y bookkeeping por cruce.

#### `level3/ntreal/ntreal`

- Estadisticas de render casi identicas.
  - Viejo: `Sphere tests=9,769,816`, `Quadric tests=15,345,979`, `Bounds tests=1,450,525`
  - Nuevo: `Sphere tests=9,768,943`, `Quadric tests=15,336,887`, `Bounds tests=1,450,382`
- Hotspots 7d9703f:
  - `Quadric::allIntersectionsForMaterial`: `51.28%`
  - `Sphere::allIntersectionsForMaterial`: `17.95%`
  - `SimpleBody::allIntersections`: `7.69%`
- Hotspots 9cff86e:
  - `CsgOperand::doIntersectionForAllRayCrossings`: `33.33%`
  - `Quadric::doIntersectionForAllRayCrossings`: `25.00%`
  - `RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)`: `7.29%`
  - `PriorityQueuePool::push/pop`: `9.37%` combinado
  - `SimpleBody::doIntersectionForAllRayCrossings`: `3.12%`
- Lectura:
  - El trabajo sobre cuadrics sigue ahi, pero ahora queda envuelto por la ruta matricial y el costo de infraestructura sube fuerte sin cambio apreciable en cantidad de pruebas.

#### `level3/piece3/piece3`

- Estadisticas de render casi identicas.
  - Viejo: `Sphere tests=45,864,370`, `Quadric tests=11,221,831`, `Bounds tests=6,527,905`
  - Nuevo: `Sphere tests=45,881,480`, `Quadric tests=11,222,043`, `Bounds tests=6,530,388`
- Hotspots 7d9703f:
  - `Sphere::allIntersectionsForMaterial`: `25.23%`
  - `Quadric::allIntersectionsForMaterial`: `13.51%`
  - `PriorityQueuePool::pop`: `10.81%`
  - `BoundedGeometry::allIntersections`: `10.81%`
  - `SimpleBody::allIntersections`: `9.01%`
  - `Composite::allIntersections`: `6.31%`
- Hotspots 9cff86e:
  - `CsgOperand::doIntersectionForAllRayCrossings`: `18.35%`
  - `SimpleBody::doIntersectionFirstHit`: `10.79%`
  - `PriorityQueuePool::push`: `10.07%`
  - `RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)`: `9.35%`
  - `SimpleBody::doIntersectionForAllRayCrossings`: `8.63%`
  - `Sphere::doIntersectionForAllRayCrossings`: `7.19%`
  - `Composite::copy`: `3.60%`
  - `java::ArrayList<Material*>::init`: `3.24%`
- Lectura:
  - Ademas del overhead matricial usual, aparece costo visible en `Composite::copy`, que no destacaba en el commit viejo. Esta escena parece especialmente sensible a la nueva composicion/copia de bodies.

#### `level2/iortest`

- Las estadisticas nuevas son incluso un poco menores que las viejas.
  - Viejo: `Sphere tests=22,440,498`, `Plane tests=24,682,753`, `Quadric tests=11,220,249`
  - Nuevo: `Sphere tests=22,212,774`, `Plane tests=24,432,261`, `Quadric tests=11,106,387`
- Hotspots 7d9703f:
  - `InfinitePlane::allIntersectionsForMaterial`: `11.76%`
  - `Quadric::allIntersectionsForMaterial`: `11.76%`
  - `ConstructiveSolidGeometryByMorganRules::allCsgIntersectIntersections`: `11.11%`
  - `SimpleBody::allIntersections`: `9.15%`
  - `Sphere::allIntersectionsForMaterial`: `7.84%`
- Hotspots 9cff86e:
  - `CsgOperand::doIntersectionForAllRayCrossings`: `22.25%`
  - `InfinitePlane::doIntersectionForAllRayCrossings`: `15.73%`
  - `PriorityQueuePool::push/pop`: `13.48%` combinado
  - `Sphere::doIntersectionForAllRayCrossings`: `6.74%`
  - `SimpleBody::doIntersectionForAllRayCrossings`: `5.62%`
  - `ConstructiveSolidGeometryByMorganRules::allCsgIntersectIntersections`: `5.39%`
  - `RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)`: `4.04%`
- Lectura:
  - Esta escena no sugiere "mas geometria"; sugiere costo adicional por la nueva ruta de CSG/owner transforms y por la infraestructura de colas/clones.

#### `level1/shapes2`

- Aqui si hay un incremento pequeno en algunas cuentas, pero no suficiente para justificar por si solo el factor `2.210x`.
  - Viejo: `Sphere tests=1,509,557`, `Triangle tests=1,307,712`, `Bounds tests=1,509,557`
  - Nuevo: `Sphere tests=1,509,973`, `Triangle tests=1,357,540`, `Bounds tests=1,509,973`
- Hotspots 7d9703f:
  - `BoundedGeometry::allIntersections`: `21.43%`
  - `Sphere::allIntersectionsForMaterial`: `21.43%`
  - `TransformableElement::doIntersectionFirstHit`: `14.29%`
  - `SimpleBody::allIntersections`: `7.14%`
- Hotspots 9cff86e:
  - `SimpleBody::doIntersectionForAllRayCrossings`: `23.08%`
  - `SimpleBody::doIntersectionFirstHit`: `19.23%`
  - `InfinitePlane::doIntersectionForAllRayCrossings`: `11.54%`
  - `PriorityQueuePool::push`: `7.69%`
  - `Sphere::doIntersectionForAllRayCrossings`: `7.69%`
  - `Triangle::doIntersectionForAllRayCrossings`: `7.69%`
  - `RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)`: `3.85%`
  - `CsgOperand::doIntersectionForAllRayCrossings`: `3.85%`
- Lectura:
  - Incluso en una escena mas pequena, el peso se mueve desde las primitivas hacia `SimpleBody` y su manejo de intersecciones. Eso refuerza la hipotesis de overhead estructural y no de una sola primitive lenta.

## Mayores regresiones

| Test | Escena | Factor |
| ---: | --- | ---: |
| 57 | `level2/spline` | 8.035 |
| 73 | `level3/ntreal/ntreal` | 3.813 |
| 79 | `level3/piece3/piece3` | 3.169 |
| 44 | `level2/iortest` | 2.985 |
| 23 | `level1/shapes2` | 2.841 |
| 52 | `level2/poolball` | 2.827 |
| 12 | `level1/dodec2` | 2.811 |
| 53 | `level2/romo` | 2.786 |
| 37 | `level2/cluster` | 2.716 |
| 69 | `level3/fishbowl` | 2.668 |

## Tabla completa

| Test | Escena | 7d9703f (s) | 9cff86e (s) | Factor |
| ---: | --- | ---: | ---: | ---: |
| 1 | `level1/bumpmap/bumpmap` | 1.266 | 1.643 | 1.298 |
| 2 | `level1/alphafun` | 2.336 | 3.285 | 1.406 |
| 3 | `level1/ballbox1` | 7.641 | 11.685 | 1.529 |
| 4 | `level1/basicvue` | 0.678 | 1.148 | 1.692 |
| 5 | `level1/blob` | 0.770 | 0.887 | 1.152 |
| 6 | `level1/box` | 0.359 | 0.537 | 1.496 |
| 7 | `level1/cantelop` | 0.359 | 0.730 | 2.033 |
| 8 | `level1/checker2` | 2.714 | 3.634 | 1.339 |
| 9 | `level1/cliptst2` | 0.989 | 2.085 | 2.109 |
| 10 | `level1/colors` | 5.614 | 13.199 | 2.351 |
| 11 | `level1/dish` | 0.844 | 1.851 | 2.194 |
| 12 | `level1/dodec2` | 0.662 | 1.860 | 2.811 |
| 13 | `level1/fogtst` | 0.758 | 1.225 | 1.616 |
| 14 | `level1/glasdish` | 11.868 | 20.290 | 1.710 |
| 15 | `level1/glass` | 13.350 | 15.144 | 1.134 |
| 16 | `level1/imagetst` | 1.619 | 2.601 | 1.607 |
| 17 | `level1/intee1` | 7.158 | 8.500 | 1.187 |
| 18 | `level1/laser` | 11.141 | 14.588 | 1.309 |
| 19 | `level1/mapper` | 0.459 | 0.672 | 1.465 |
| 20 | `level1/mappr2` | 0.563 | 0.996 | 1.770 |
| 21 | `level1/matmap` | 1.220 | 1.826 | 1.497 |
| 22 | `level1/pvinterp` | 0.690 | 1.109 | 1.607 |
| 23 | `level1/shapes2` | 1.931 | 5.484 | 2.841 |
| 24 | `level1/shapes` | 3.743 | 8.181 | 2.185 |
| 25 | `level1/spotlite` | 1.879 | 2.635 | 1.402 |
| 26 | `level1/stone1` | 2.655 | 5.033 | 1.896 |
| 27 | `level1/stone2` | 6.142 | 10.582 | 1.723 |
| 28 | `level1/stone3` | 5.309 | 9.379 | 1.767 |
| 29 | `level1/stone4` | 0.562 | 0.843 | 1.498 |
| 30 | `level1/sunset1` | 1.183 | 1.975 | 1.670 |
| 31 | `level1/sunset` | 4.300 | 5.519 | 1.283 |
| 32 | `level1/texture1` | 3.729 | 6.918 | 1.855 |
| 33 | `level1/texture2` | 5.964 | 12.263 | 2.056 |
| 34 | `level1/texture3` | 2.016 | 3.451 | 1.711 |
| 35 | `level1/window` | 1.437 | 2.594 | 1.805 |
| 36 | `level2/arches` | 12.948 | 16.167 | 1.249 |
| 37 | `level2/cluster` | 2.081 | 5.652 | 2.716 |
| 38 | `level2/crystal` | 6.310 | 11.111 | 1.761 |
| 39 | `level2/eight` | 1.114 | 2.028 | 1.822 |
| 40 | `level2/esp01` | 2.688 | 3.539 | 1.316 |
| 41 | `level2/hfclip` | 2.457 | 3.602 | 1.466 |
| 42 | `level2/illum1` | 17.038 | 36.670 | 2.152 |
| 43 | `level2/illum2` | 5.115 | 10.034 | 1.962 |
| 44 | `level2/iortest` | 32.231 | 96.212 | 2.985 |
| 45 | `level2/lpops1` | 6.262 | 10.090 | 1.611 |
| 46 | `level2/lpops2` | 7.584 | 12.782 | 1.685 |
| 47 | `level2/magglass` | 2.727 | 4.772 | 1.750 |
| 48 | `level2/mtmand` | 1.390 | 2.008 | 1.444 |
| 49 | `level2/pacman` | 1.693 | 3.489 | 2.060 |
| 50 | `level2/pawns` | 30.448 | 47.422 | 1.558 |
| 51 | `level2/planet` | 3.442 | 4.032 | 1.171 |
| 52 | `level2/poolball` | 1.635 | 4.621 | 2.827 |
| 53 | `level2/romo` | 2.611 | 7.274 | 2.786 |
| 54 | `level2/room` | 3.675 | 5.741 | 1.562 |
| 55 | `level2/skyvase` | 8.157 | 15.837 | 1.941 |
| 56 | `level2/smoke` | 2.782 | 3.497 | 1.257 |
| 57 | `level2/spline` | 6.347 | 50.998 | 8.035 |
| 58 | `level2/stonewal` | 3.810 | 7.181 | 1.885 |
| 59 | `level2/sunsethf` | 6.331 | 8.161 | 1.289 |
| 60 | `level2/tetra` | 5.770 | 11.952 | 2.072 |
| 61 | `level2/waterbow` | 5.408 | 9.235 | 1.708 |
| 62 | `level2/wtorus` | 2.786 | 2.898 | 1.040 |
| 63 | `level3/car/car` | 12.922 | 31.436 | 2.433 |
| 64 | `level3/chess` | 6.007 | 12.749 | 2.123 |
| 65 | `level3/desk` | 7.898 | 19.975 | 2.529 |
| 66 | `level3/dfwood` | 3.539 | 5.084 | 1.437 |
| 67 | `level3/drums2/drums` | 91.379 | 238.129 | 2.606 |
| 68 | `level3/fish13/fish13` | 13.750 | 30.778 | 2.238 |
| 69 | `level3/fishbowl` | 3.203 | 8.546 | 2.668 |
| 70 | `level3/ionic5/ionic5` | 26.080 | 63.680 | 2.442 |
| 71 | `level3/kscope` | 7.797 | 12.612 | 1.618 |
| 72 | `level3/lamp` | 3.778 | 7.710 | 2.041 |
| 73 | `level3/ntreal/ntreal` | 8.002 | 30.514 | 3.813 |
| 74 | `level3/oak2` | 24.775 | 34.210 | 1.381 |
| 75 | `level3/palace` | 3.252 | 6.224 | 1.914 |
| 76 | `level3/pencil/pencil` | 4.344 | 10.101 | 2.325 |
| 77 | `level3/piece1` | 13.532 | 30.414 | 2.248 |
| 78 | `level3/piece2/piece2` | 23.285 | 48.280 | 2.073 |
| 79 | `level3/piece3/piece3` | 24.197 | 76.681 | 3.169 |
| 80 | `level3/pool` | 4.903 | 10.273 | 2.095 |
| 81 | `level3/roman` | 7.760 | 16.443 | 2.119 |
| 82 | `level3/snack` | 25.987 | 55.030 | 2.118 |
| 83 | `level3/snail/snail` | 13.195 | 29.102 | 2.205 |
| 84 | `level3/takeoff` | 8.087 | 13.244 | 1.638 |
| 85 | `level3/teapot/teapot` | 11.389 | 12.326 | 1.082 |
| 86 | `level3/tomb` | 13.586 | 30.672 | 2.258 |
| 87 | `level3/wealth` | 9.820 | 21.789 | 2.219 |
| 88 | `level3/wg5` | 18.850 | 36.965 | 1.961 |
| 89 | `math/bezier0` | 0.564 | 0.593 | 1.051 |
| 90 | `math/bezier` | 1.452 | 1.752 | 1.206 |
| 91 | `math/bicube` | 0.804 | 0.856 | 1.064 |
| 92 | `math/folium` | 1.283 | 1.836 | 1.431 |
| 93 | `math/grafbic` | 2.279 | 2.812 | 1.234 |
| 94 | `math/helix` | 3.528 | 5.693 | 1.614 |
| 95 | `math/hyptorus` | 1.727 | 2.165 | 1.254 |
| 96 | `math/lemnisc2` | 2.710 | 3.755 | 1.386 |
| 97 | `math/lemnisca` | 1.112 | 1.517 | 1.365 |
| 98 | `math/monkey` | 1.033 | 1.764 | 1.707 |
| 99 | `math/partorus` | 1.945 | 2.439 | 1.254 |
| 100 | `math/piriform` | 1.226 | 1.727 | 1.409 |
| 101 | `math/quarcyl` | 0.807 | 1.228 | 1.521 |
| 102 | `math/quarpara` | 2.505 | 3.771 | 1.506 |
| 103 | `math/steiner` | 1.582 | 2.173 | 1.373 |
| 104 | `math/tcubic` | 1.078 | 1.470 | 1.364 |
| 105 | `math/teardrop` | 0.928 | 0.943 | 1.016 |
| 106 | `math/torus` | 1.179 | 1.663 | 1.411 |
| 107 | `math/trough` | 1.981 | 3.120 | 1.575 |
| 108 | `math/witch` | 1.722 | 2.443 | 1.419 |
