# Performance Review Plan 4

## Scope

This plan continues the investigation from `doc/performanceReviewPlan3.md`.
Plans 1 through 3 brought the scene-owned baked CSG representation from an
initial 2.3× slower state at the start of Plan 2 down to approximately 1.68×
slower than the older baked baseline on `level3/drums2/drums.pov` at `320x200`.

Reference commits:

- Older baked baseline:
  `4af1a75e5b7356600bec34e12a4882560994a058`
- Scene-owned baked branch under comparison:
  `0d2a21cdfec10d3b3e7d41b53bba6e08333a08be`

Primary benchmark:

- Scene: `level3/drums2/drums.pov`
- Resolution: `320x200`
- Output format: TGA
- Timing mode: optimized `Release` build for wall-clock measurements,
  `-pg` build for `gprof` attribution

Secondary benchmark panel (five scenes, unchanged from Plan 1):

- `level2/spline`
- `level3/ntreal/ntreal`
- `level3/piece3/piece3`
- `level2/iortest`
- `level1/shapes2`

## Current State at `0d2a21c`

The following work was completed across Plans 1–3 and is included in the
comparison branch:

- Explicit pre-render CSG execution-plan metadata (`BakedCsgExecutionPlanKind`)
  with compiled operand-role lists and plan-inventory statistics
- Compiled core-plus-plane all-crossings and first-hit paths inside
  `BakedCsgTracing`
- Transformed-nested descriptor compilation with pre-copied plane roles
  (`compiledNestedPlaneOperandIndices`) and containment sequences
  (`compiledNestedContainmentOperandIndices`)
- `trueMiss` early-exit for direct-quadric core operands, with inlined
  containment via `candidateInsideDirectDescriptorOperands`
- Transformed-nested core-plane emitter
  (`traceTransformedNestedSingleCorePlaneOperandAllCrossings`) active for
  non-primary rays via parent-plan dispatch (`tracePlanOperandAllCrossings`)
- Phase C regression (`!isPrimaryRayEnabled()` gate removed) partially
  recovered by Path G (gate restored on `tracePlanOperandAllCrossings`)
- Viewpoint cache added to all baked quadric intersection helpers (Phase E)
- `bakedScene` package refactor: `Baked*.cpp/h` moved to
  `src/render/bakedScene/`; anonymous namespaces replaced with private static
  methods; `CsgScratchContext` extracted to its own header

Measured state at `0d2a21c` vs `4af1a75` on `drums` at `320x200`:

| Commit | Meaning | Time (s) | Ratio |
| --- | --- | ---: | ---: |
| `4af1a75` | Older baked baseline | ~5.10 | 1.00 |
| `0d2a21c` | Scene-owned baked current | ~8.55 | 1.68 |

Known image delta between the two commits: `AE=71`, `RMSE≈0.00526`. This is a
stable, accepted difference — not a correctness regression introduced by the
performance work.

Full-suite correctness gate at `0d2a21c`: `Test passed.` with the following
accepted non-zero diffs:
`level1/ballbox1 AE=8`, `level1/texture3 AE=327`, `level2/illum2 AE=21`,
`level2/pawns AE=5`, `level3/car AE=6`, `level3/ionic5 AE=145114`,
`math/folium AE=10`, `math/helix AE=92`, `math/monkey AE=21`,
`math/quarpara AE=133`, `math/tcubic AE=48`, `math/trough AE=155`,
`math/witch AE=175`.

Key `gprof` counters at `0d2a21c` (from Plan 3, Phase F):

| Counter | Value |
| --- | ---: |
| `traceOperandAllCrossings` calls | 79.3M |
| `traceTransformedNestedSingleCorePlaneOperandAllCrossings` calls | 24.8M |
| `LocalIntersectionClone` calls | 50.2M |
| `candidateInsideDirectDescriptorOperands` calls | 53.1M |
| `PriorityQueuePool::pop` calls | 61.3M |

`LocalIntersectionClone` attribution: 40% from `traceOperandAllCrossings`,
34% from `BakedSimpleBodyTracing::traceAllCrossings`, 23% from
`BakedCompositeTracing::traceAllCrossings`.

## Architectural Constraint

The ownership boundary established in Plans 2 and 3 remains in effect:

- `environment/geometry`: primitive math and geometry-local intersection routines
- `environment/scene`: parsed scene ownership plus immutable tracing compilation,
  including composed transforms, baked CSG records, stable flags, bounds, and
  traversal metadata
- `render`: consumes the scene-owned baked representation without mutating
  geometry objects

Any optimization that moves transform baking or mutable render-specific state
back into `Geometry` must be rejected or redesigned as a scene-owned baked
specialization.

## Central Question

How much of the remaining 1.68× gap between `0d2a21c` and `4af1a75` can be
closed by further baking or structural compilation in `environment/scene` and
`render/bakedScene`, without crossing the ownership boundary above?

## Pending Directions From Plan 3

The following were identified but not yet executed at the end of Plan 3:

- **Path H-bis**: extract `Sphere::intersectSphereLocalSpace` as a public
  static method so that transformed `Sphere` CSG operands can be intersected
  without a `LocalIntersectionClone`; expected to eliminate ~20M clones from
  `traceOperandAllCrossings`
- **Other `TransformedPrimitive` kinds** (Box, Torus, etc.) that still create
  clones but are less hot than Sphere in `drums`
- **`BakedSimpleBodyTracing::traceAllCrossings`** and
  **`BakedCompositeTracing::traceAllCrossings`** clone paths, accounting for
  34% and 23% of the remaining 50.2M `LocalIntersectionClone` calls
  respectively; these require a different approach from the CSG-emitter
  techniques already used

---

## Gprof Sweep: All 108 Scenes at 320×200 (2026-07-03)

### Methodology

- Binary: `0d2a21c`, Release build with `-pg` instrumentation
- Each of the 108 scenes ran sequentially at `320x200` in an isolated directory
  so `gmon.out` files never overlap
- 105 scenes produced a meaningful flat profile (≥10 ms attributed time);
  3 scenes (`pvinterp`, `teardrop`, and one other) had zero accumulated time
  under the 10 ms sampling resolution
- Total attributed self-time across all 105 profiles: **28.97 s**
- For scenes with local include files (`drums.inc`, `ntreal.inc`, `fish.inc`,
  `teapot.inc`, `mmshape.inc`, `light.inc`, `axisbox.inc`), the binary ran
  from the scene's own directory so relative paths resolved correctly
- For scenes with data-file references (`bumpmap_.gif`, `plasma2.gif`,
  `pacman.iff`, `fract003.gif`, `eight_.iff`), the same convention applied
- Release binary was rebuilt after profiling to restore the non-instrumented
  executable

### Top Functions — 98% Coverage

The table below lists the 93 functions whose cumulative self-time reaches 98%
of the 28.97 s total. Functions are grouped by category. Within each category,
rows are ordered by descending self-time.

`Calls (M)` = total calls summed across all 105 scenes.
`Layer` abbreviation key:

- **CSG-D** CSG all-crossings dispatcher
- **Body** BakedSimpleBodyTracing traversal
- **Comp** BakedCompositeTracing traversal
- **BTC** BakedTracingCommon + RenderEngine trace path
- **Prim** primitive intersection math
- **Cont** CSG containment / membership / plane-candidate logic
- **Queue** PriorityQueuePool and java::ArrayList/PriorityQueue
- **Ray** RayWithSegments construction / cloning
- **Shade** shading and material pipeline
- **I/O** scene construction, I/O, ray creation

| Rk | Self (s) | % | Cum% | Calls (M) | Layer | Function |
| ---: | ---: | ---: | ---: | ---: | --- | --- |
| 1 | 2.15 | 7.42 | 7.42 | 244.5 | CSG-D | `BakedCsgTracing::traceOperandAllCrossings` |
| 2 | 1.50 | 5.18 | 12.60 | 159.7 | Body | `BakedSimpleBodyTracing::traceAllCrossings` |
| 3 | 0.93 | 3.21 | 15.81 | 74.0 | CSG-D | `BakedCsgTracing::traceGenericMorganUnion` |
| 4 | 0.88 | 3.04 | 18.85 | 303.7 | Queue | `PriorityQueuePool::push` |
| 5 | 0.84 | 2.90 | 21.75 | 295.3 | Queue | `PriorityQueuePool::pop` |
| 6 | 0.84 | 2.90 | 24.65 | 20.0 | CSG-D | `BakedCsgTracing::traceMorganIntersectionGeneric` |
| 7 | 0.84 | 2.90 | 27.55 | 62.5 | Prim | `BakedCsgTracing::intersectBakedQuadricWithTrueMiss` |
| 8 | 0.78 | 2.69 | 30.24 | 121.7 | Body | `BakedSimpleBodyTracing::traceFirstHit` |
| 9 | 0.77 | 2.66 | 32.90 | 260.9 | Ray | `RayWithSegments::RayWithSegments (LocalIntersectionClone)` |
| 10 | 0.74 | 2.55 | 35.45 | 90.4 | Body | `BakedSimpleBodyTracing::traceFirstHit::{lambda}` |
| 11 | 0.68 | 2.35 | 37.80 | 238.6 | Body | `BakedSimpleBodyTracing::passesBoundingShapes` |
| 12 | 0.67 | 2.31 | 40.11 | 47.3 | Comp | `BakedCompositeTracing::traceAllCrossingsInCompositeSpace` |
| 13 | 0.65 | 2.24 | 42.35 | 46.7 | Cont | `BakedCsgTracing::tracePlaneOperandCandidate` |
| 14 | 0.63 | 2.17 | 44.53 | 6.2 | Prim | `PolynomialShape::intersectQuartic` |
| 15 | 0.59 | 2.04 | 46.57 | 118.3 | Prim | `BakedCsgTracing::rayIntersectsAabbForward` |
| 16 | 0.56 | 1.93 | 48.50 | 9.4 | Shade | `DirectLightShader::shade` |
| 17 | 0.54 | 1.86 | 50.36 | 61.0 | Body | `BakedSimpleBodyTracing::finalizeCandidate` |
| 18 | 0.53 | 1.83 | 52.19 | 34.1 | Prim | `BakedCsgTracing::intersectBakedQuadric` |
| 19 | 0.53 | 1.83 | 54.02 | 42.9 | CSG-D | `BakedCsgTracing::traceTransformedNestedSingleCorePlaneOperandAllCrossings` |
| 20 | 0.52 | 1.79 | 55.82 | 59.7 | CSG-D | `BakedCsgTracing::traceAllCrossings` |
| 21 | 0.46 | 1.59 | 57.40 | 260.9 | Queue | `java::ArrayList::init` |
| 22 | 0.44 | 1.52 | 58.92 | 123.0 | Prim | `BakedCsgTracing::intersectBakedPlane` |
| 23 | 0.43 | 1.48 | 60.41 | 165.0 | Queue | `java::PriorityQueue::siftUp` |
| 24 | 0.42 | 1.45 | 61.86 | 266.2 | Queue | `java::ArrayList::ArrayList` |
| 25 | 0.41 | 1.42 | 63.27 | 85.7 | Cont | `BakedCsgTracing::mergeByMembership` |
| 26 | 0.39 | 1.35 | 64.62 | 129.8 | CSG-D | `BakedCsgTracing::traceMorganCsg` |
| 27 | 0.38 | 1.31 | 65.93 | 40.2 | CSG-D | `BakedCsgTracing::traceCompiledCoreOperandAllCrossings` |
| 28 | 0.36 | 1.24 | 67.17 | — | I/O | `Scene::Scene` (construction) |
| 29 | 0.35 | 1.21 | 68.38 | 82.7 | BTC | `::traceShadowObject` |
| 30 | 0.34 | 1.17 | 69.55 | 8.5 | CSG-D | `BakedCsgTracing::traceFirstHitByIntersectionMembership` |
| 31 | 0.32 | 1.10 | 70.66 | 11.6 | BTC | `RenderEngine::trace` |
| 32 | 0.30 | 1.04 | 71.69 | 15.8 | Prim | `Quadric::doIntersectionForAllRayCrossingsAnnotated` |
| 33 | 0.29 | 1.00 | 72.70 | 38.6 | Comp | `BakedCompositeTracing::traceAllCrossings` |
| 34 | 0.27 | 0.93 | 73.63 | 18.6 | CSG-D | `BakedCsgTracing::traceCompiledCoreFirstHitCandidates` |
| 35 | 0.27 | 0.93 | 74.56 | 20.0 | Prim | `Sphere::doIntersectionFirstHitNoQueue` |
| 36 | 0.26 | 0.90 | 75.46 | 20.8 | Prim | `Sphere::doIntersectionForAllRayCrossings` |
| 37 | 0.25 | 0.86 | 76.32 | 198.3 | BTC | `BakedTracingCommon::traceObjectAllCrossings` |
| 38 | 0.25 | 0.86 | 77.18 | 16.8 | Prim | `Quadric::doIntersectionForAllRayCrossings` |
| 39 | 0.24 | 0.83 | 78.01 | 8.6 | Body | `SimpleBody::doExtraInformation` |
| 40 | 0.23 | 0.79 | 78.81 | 32.9 | Comp | `BakedCompositeTracing::rayIntersectsAabbForward` |
| 41 | 0.22 | 0.76 | 79.57 | 17.4 | Shade | `RayShaderPipeline::shadeSurface` |
| 42 | 0.20 | 0.69 | 80.26 | 80.9 | Cont | `BakedCsgTracing::containmentTestOperand` |
| 43 | 0.20 | 0.69 | 80.95 | 61.9 | Cont | `Quadric::doContainmentTest` |
| 44 | 0.19 | 0.66 | 81.60 | 34.6 | Ray | `RayWithSegments::makeRay` |
| 45 | 0.19 | 0.66 | 82.26 | 31.4 | CSG-D | `BakedCsgTracing::traceFirstHit` |
| 46 | 0.19 | 0.66 | 82.91 | 10.5 | Prim | `BakedCsgTracing::intersectBakedQuadricWithCoeffs` |
| 47 | 0.18 | 0.62 | 83.53 | 28.3 | Queue | `java::PriorityQueue::add` |
| 48 | 0.18 | 0.62 | 84.16 | 27.9 | Cont | `BakedCsgTracing::containmentTest` |
| 49 | 0.18 | 0.62 | 84.78 | 34.8 | Body | `BakedSimpleBodyTracing::containmentTest` |
| 50 | 0.17 | 0.59 | 85.36 | 14.8 | Shade | `ColorOperations::clipColor` |
| 51 | 0.16 | 0.55 | 85.92 | 8.7 | Comp | `BakedCompositeTracing::traceFirstHit` |
| 52 | 0.16 | 0.55 | 86.47 | 18.6 | CSG-D | `BakedCsgTracing::traceFirstHitCompiledSingleCorePlane` |
| 53 | 0.15 | 0.52 | 86.99 | 1.0 | Prim | `PolynomialShape::evaluatePolynomial` |
| 54 | 0.15 | 0.52 | 87.50 | 8.7 | Cont | `BakedCsgTracing::tracePlaneOperandCandidateInRaySpace` |
| 55 | 0.14 | 0.48 | 87.99 | 25.0 | Cont | `BakedCsgTracing::traceTransformedQuadricCorePlaneIntersection` |
| 56 | 0.14 | 0.48 | 88.47 | 11.0 | Shade | `LambertShader::shade` |
| 57 | 0.13 | 0.45 | 88.92 | 31.4 | CSG-D | `BakedCsgTracing::traceFirstHitWithScratch` |
| 58 | 0.13 | 0.45 | 89.37 | 17.7 | Cont | `BakedCsgTracing::offerTransformedPrimitiveCandidate` |
| 59 | 0.13 | 0.45 | 89.82 | 6.2 | Prim | `Sphere::intersectSphereLocalSpace` |
| 60 | 0.13 | 0.45 | 90.27 | 0.8 | Prim | `ParametricBiCubicPatch::parametricSubDivider` |
| 61 | 0.13 | 0.45 | 90.71 | 16.9 | Prim | `InfinitePlane::doIntersectionForAllRayCrossings` |
| 62 | 0.13 | 0.45 | 91.16 | 6.3 | I/O | `RenderEngine::createRay` |
| 63 | 0.12 | 0.41 | 91.58 | 9.4 | Shade | `LocalSurfaceShader::shade` |
| 64 | 0.11 | 0.38 | 91.96 | — | I/O | `RenderTargetImage::allocate` |
| 65 | 0.10 | 0.35 | 92.30 | 23.1 | Queue | `java::ArrayList::dispose` |
| 66 | 0.10 | 0.35 | 92.65 | 7.4 | Prim | `Quadric::doIntersectionFirstHitNoQueue` |
| 67 | 0.09 | 0.31 | 92.96 | 16.1 | Shade | `LightSamplerShader::sample` |
| 68 | 0.09 | 0.31 | 93.27 | 5.0 | Cont | `BakedCsgTracing::offerCompiledSingleCorePlaneFirstHitCandidate` |
| 69 | 0.09 | 0.31 | 93.58 | 3.1 | Prim | `ParametricBiCubicPatch::parametricBoundingSphere` |
| 70 | 0.09 | 0.31 | 93.89 | 101.5 | BTC | `BakedTracingCommon::traceObjectFirstHit` |
| 71 | 0.08 | 0.28 | 94.17 | 46.2 | Queue | `java::ArrayList::~ArrayList` |
| 72 | 0.08 | 0.28 | 94.44 | 5.3 | Prim | `PolynomialShape::doIntersectionForAllRayCrossings` |
| 73 | 0.07 | 0.24 | 94.68 | 19.2 | Shade | `SolidTexturePigment::transformToObjectSpace` |
| 74 | 0.07 | 0.24 | 94.93 | 9.4 | Shade | `MirrorReflectionShader::shade` |
| 75 | 0.07 | 0.24 | 95.17 | 34.8 | Body | `BakedTracingCommon::containmentTest` |
| 76 | 0.06 | 0.21 | 95.41 | 5.5 | Prim | `::patchScratchFor` |
| 77 | 0.06 | 0.21 | 95.62 | 61.5 | BTC | `LightGeometryAdapter::doIntersectionForAllRayCrossings` |
| 78 | 0.06 | 0.21 | 95.82 | — | I/O | `RenderEngine::renderTile` |
| 79 | 0.06 | 0.21 | 96.03 | 3.5 | Shade | `PhongSpecularShader::shade` |
| 80 | 0.06 | 0.21 | 96.24 | 24.4 | Cont | `BakedCsgTracing::candidateInsideCompiledSingleCorePlaneOperands` |
| 81 | 0.06 | 0.21 | 96.44 | 11.1 | Prim | `ParametricBiCubicIntersection::pointPlaneDistance` |
| 82 | 0.06 | 0.21 | 96.65 | 1.1 | Prim | `ParametricBiCubicPatch::parametricSplitUpDown` |
| 83 | 0.04 | 0.14 | 96.79 | 2.0 | Shade | `TransmissionRefractionShader::shade` |
| 84 | 0.04 | 0.14 | 96.93 | 5.6 | Prim | `Box::doIntersectionForAllRayCrossings` |
| 85 | 0.04 | 0.14 | 97.07 | 2.8 | Body | `CsgOperand::doExtraInformation` |
| 86 | 0.04 | 0.14 | 97.20 | 0.3 | I/O | `Tokenizer::findReserved` |
| 87 | 0.04 | 0.14 | 97.34 | — | I/O | `::compileConstructiveSolidGeometry` |
| 88 | 0.04 | 0.14 | 97.48 | 2.6 | Prim | `Box::doIntersectionForAllRayCrossingsAnnotated` |
| 89 | 0.03 | 0.10 | 97.58 | 9.8 | Prim | `InfinitePlane::doIntersectionFirstHitNoQueue` |
| 90 | 0.03 | 0.10 | 97.69 | 0.3 | I/O | `DataFile::readSymbol` |
| 91 | 0.03 | 0.10 | 97.79 | — | I/O | `TargaFormat::writeLine` |
| 92 | 0.03 | 0.10 | 97.89 | — | I/O | `::defaultViewPoint` |
| 93 | 0.03 | 0.10 | 98.00 | 6.8 | Cont | `InfinitePlane::doContainmentTest` |

### Category Summary

| Category | Self (s) | % | Key functions (first 2–3) |
| --- | ---: | ---: | --- |
| CSG-D: all-crossings dispatchers | 6.84 | 23.6 | `traceOperandAllCrossings` 7.4%, `traceGenericMorganUnion` 3.2%, `traceMorganIntersectionGeneric` 2.9% |
| Body/Comp/BTC: traversal scaffolding | 6.83 | 23.6 | `BakedSimpleBodyTracing::traceAllCrossings` 5.2%, `BakedSimpleBodyTracing::traceFirstHit` 2.7%, `BakedCompositeTracing::traceAllCrossingsInCompositeSpace` 2.3% |
| Prim: primitive intersection math | 5.35 | 18.5 | `intersectBakedQuadricWithTrueMiss` 2.9%, `PolynomialShape::intersectQuartic` 2.2%, `rayIntersectsAabbForward` 2.0% |
| Queue: PriorityQueuePool + java containers | 3.39 | 11.7 | `PriorityQueuePool::push` 3.0%, `PriorityQueuePool::pop` 2.9%, `java::PriorityQueue::siftUp` 1.5% |
| Cont: containment / plane-candidate / merge | 2.25 | 7.8 | `tracePlaneOperandCandidate` 2.2%, `mergeByMembership` 1.4%, `containmentTestOperand` 0.7% |
| Shade: shading pipeline | 1.54 | 5.3 | `DirectLightShader::shade` 1.9%, `RayShaderPipeline::shadeSurface` 0.8%, `ColorOperations::clipColor` 0.6% |
| Ray: cloning + construction | 0.96 | 3.3 | `RayWithSegments::RayWithSegments (clone)` 2.7%, `RayWithSegments::makeRay` 0.7% |
| I/O: scene init, ray creation, output | 0.81 | 2.8 | `Scene::Scene` 1.2%, `RenderEngine::createRay` 0.5%, `RenderTargetImage::allocate` 0.4% |
| **Total** | **28.97** | **96.6** | *(remaining 3.4% in minor functions below 0.10%)* |

### Scene Groups by Dominant Hotspot Pattern

Instrumented wall-clock is 2–3× real time. The group composition reflects which
cost layer dominates, which is what matters for optimization targeting.

| Group | Defining bottleneck | Scenes (instr. s) | Group total |
| --- | --- | --- | ---: |
| **A: CSG-heavy** | `traceOperandAllCrossings` ≥9% | drums (5.88), iortest (2.07), snack (1.59), snail (0.78), wg5 (0.95), tomb (0.55), desk (0.55), chess (0.30), skyvase (0.47), romo (0.19), pool (0.19), fishbowl (0.25), palace (0.18), tetra (0.27), ballbox1 (0.26), waterbow (0.15), cluster (0.11), texture1 (0.12) | **14.6 s** |
| **B: Simple-body / composite traversal** | `BakedSimpleBodyTracing::traceAllCrossings` ≥10% or `BakedCompositeTracing` dominant | piece2 (1.26), fish13 (0.75), roman (0.47), piece1 (0.64), lpops1 (0.28), lpops2 (0.27), kscope (0.28), wealth (0.37), laser (0.28), shapes (0.20), ntreal (0.42), illum1 (0.76) | **6.0 s** |
| **C: Polynomial solver** | `PolynomialShape::intersectQuartic` ≥8% | oak2 (0.93), pawns (0.86), helix (0.16), lemnisc2 (0.12), witch (0.08), tcubic (0.07), partorus (0.05), hyptorus (0.04), steiner (0.04) | **2.4 s** |
| **D: Parametric patch** | `ParametricBiCubicPatch::parametricSubDivider` dominant | teapot (0.56), bezier (0.09), bezier0 (0.04) | **0.7 s** |
| **E: Sphere/shading-heavy** | `Sphere::doIntersectionFirstHitNoQueue` or `DirectLightShader::shade` ≥15% | glasdish (0.47), illum2 (0.32), crystal (0.38), colors (0.08), room (0.12) | **1.4 s** |
| **F: Fast / I/O-dominated** | gprof sampling noise; scene init cost visible | piece3 (0.01), pencil (0.01), car (0.01), ionic5 (0.02), mtmand (0.01), poolball (0.01) | **~0.1 s** |

### Diagnostic Observations

**1. Time is fragmented across 93 functions; the baseline had 3.**

The older baked baseline (`4af1a75`) concentrated 62% of time in three tight
loops:

| Function | % time |
| --- | ---: |
| `ConstructiveSolidGeometryByMorganRules::allCsgIntersectIntersections` | 24.9 |
| `Quadric::doIntersectionForAllRayCrossings` | 23.0 |
| `InfinitePlane::doIntersectionForAllRayCrossings` | 14.4 |

The current branch spreads the equivalent work across 93 functions, none above
7.4%. The root cause is that each ray visit to a CSG object now passes through
multiple dispatch layers (`traceAllCrossings` → `traceMorganCsg` →
`traceOperandAllCrossings` → `intersectBakedQuadric*`) where the baseline
called the Morgan kernel and the primitive directly.

**2. The CSG dispatching group (CSG-D) and the traversal scaffolding group
(Body/Comp/BTC) each account for 23.6% of total time — together nearly half.**

These costs are pure routing overhead relative to the work done at each call.
`traceOperandAllCrossings` alone is called 244.5 M times. In the baseline,
the equivalent routing was inside the tight `allCsgIntersectIntersections` loop
with no virtual dispatch and no per-call operand classification.

**3. Queue overhead (11.7%) is not caused by the CSG scratch context —
it is inherent to per-primitive heap insertion.**

`PriorityQueuePool::push` and `pop` are called 303.7 M and 295.3 M times
respectively. `java::ArrayList::init` and `ArrayList::ArrayList` add 1.59%
and 1.45%. These calls arise because every primitive hit becomes a heap-inserted
candidate. The baseline also used a heap, but the per-operand dispatch was
tighter, so the total candidate count was lower.

**4. Ray cloning (3.3%) is no longer the dominant cost but is still significant.**

`RayWithSegments::RayWithSegments (LocalIntersectionClone)` at 260.9 M calls
remains the single function with the highest call count. It is not the top
cost by self-time because each clone is cheap, but eliminating it would reduce
indirect cache pressure in the surrounding dispatch.

**5. Polynomial scenes (Group C, 8.3%) are a separate problem.**

`PolynomialShape::intersectQuartic` (2.17%) is not a dispatching cost — it is
genuine solver work. It is hot only in Group C scenes (oak2, pawns, and math
scenes) and has no connection to the baked CSG gap. It is not a target for
the current plan.

**6. The `BakedSimpleBodyTracing::traceFirstHit::{lambda}` at rank 10 (2.55%)
is a pure trampoline cost.**

This is the lambda inside `traceFirstHit` that wraps the bounding-shape pass.
It is called 90.4 M times. Its presence in the top 10 indicates that the
bounding-shape test structure adds one extra indirect call per first-hit attempt
even for objects with no bounding shapes.

### Strategy Implications for This Plan

The 1.68× gap between `0d2a21c` and `4af1a75` on `drums` at `320x200` has the
following structural breakdown based on the full-suite profile:

| Cost layer | % of total (current) | % of total (baseline analogue) | Gap |
| --- | ---: | ---: | --- |
| CSG-D dispatchers | 23.6 | ~5 (inline Morgan kernel) | **−18.6 pp** |
| Body/Comp/BTC scaffolding | 23.6 | ~8 (direct scene scan) | **−15.6 pp** |
| Prim: intersection math | 18.5 | ~40 (Quadric+Plane dominated) | **+21.5 pp** |
| Queue overhead | 11.7 | ~8 | **−3.7 pp** |
| Cont: membership / merge | 7.8 | ~5 | **−2.8 pp** |
| Shade | 5.3 | ~5 | neutral |
| Ray cloning | 3.3 | ~3 | neutral |
| I/O + other | 6.2 | ~6 | neutral |

Reading: the current branch spends far more on dispatching and scaffolding
(−34 pp) and correspondingly less on primitive math (+21 pp). Closing the gap
requires reducing the per-ray call depth and total dispatch volume, not
improving primitive solvers.

The three most actionable targets in priority order:

1. **Reduce `traceOperandAllCrossings` volume (244.5 M calls, 7.4% self-time).**
   Every compiled CSG traversal still routes through this function per operand.
   A compiled execution path that bypasses the per-operand switch for the
   common `SingleCorePlaneIntersection` pattern would remove most of these calls.

2. **Reduce `BakedSimpleBodyTracing::traceAllCrossings` overhead (5.2%).**
   This function is hot across Groups A and B. The inner lambda (rank 10, 2.55%)
   adds an extra call per first-hit even with no bounding shapes. Inlining or
   restructuring the bounding-shape test would reduce both.

3. **Reduce queue insertion pressure (11.7% combined).**
   `PriorityQueuePool::push/pop` (6.0%) and `java::PriorityQueue::siftUp`
   (1.5%) arise because every primitive candidate goes through a heap. For CSG
   paths where the result is known to be order-insensitive, a fixed-size array
   pass or a sorted-on-demand vector could replace the heap without changing
   correctness.

---

### Local Time Minima

| Pos | Date | Commit | Time (s) | Subject |
| ---: | --- | --- | ---: | --- |
| 2 | 2026-06-29 16:04:36 +0200 | `9cff86e4c11499a5e17ab4906cc37befa7ad6eb3` | 248.152 | Performance review, part 2 |
| 5 | 2026-06-27 21:53:29 +0200 | `3fa7a8122ed654de54dcd59199c82f1720973acd` | 372.602 | Decoupling TransformedGeometry from Geometry, part 5 |
| 9 | 2026-06-27 17:17:57 +0200 | `3f481c4d212a0d6bd51b2b450104f706f9ec1fd5` | 129.962 | Decoupling TransformedGeometry from Geometry |
| 11 | 2026-06-27 15:36:02 +0200 | `0015768f2627e2abb1ce1979e3bf4f24d0693427` | 86.100 | Documentation update |
| 13 | 2026-06-27 13:49:56 +0200 | `8f780b8e00d9b87406f879214ea9287c27d5fe70` | 86.504 | SimpleBody centered reorganization |
| 23 | 2026-06-25 22:02:45 +0200 | `0e65ec47bfc987590064230b1c19fb1ca4b7ac68` | 95.215 | Unused constructors clean up |
| 26 | 2026-06-25 19:08:01 +0200 | `6612ce808f1f420b601e92cbcc3ff64ed3fd6597` | 95.493 | Camera from vitral being used |
| 30 | 2026-06-24 19:52:07 +0200 | `d60aedd4652daa330c32e5710507ce94a1a51cee` | 95.194 | Vitral sync, HashMap and Collections |
| 32 | 2026-06-24 17:17:44 +0200 | `cdb7e1885e6363565ea3d892806adf5ac6cc7533` | 95.592 | CSG performance review |
| 34 | 2026-06-23 08:17:14 +0200 | `6b0641f67556f82ca106f702d5c25b3c92f81524` | 97.078 | PriorityQueuePool is a template |
| 37 | 2026-06-22 21:37:13 +0200 | `4e90018ac8aa8bb7318e2aa5c586c3c389ba64fe` | 98.748 | Aligning basic raytracer classes with vitral concepts |
| 40 | 2026-06-22 13:03:39 +0200 | `631b2ac2f8264bf9887bccc7774872610a2398bd` | 98.312 | Render decoupled from render context, which is only used by io |
| 42 | 2026-06-22 11:10:07 +0200 | `ed2edd36e475612e457911cec4488fb2ef3faf3e` | 98.217 | Class forward declarations replaced by proper includes |
| 46 | 2026-06-21 22:45:46 +0200 | `c92e13af867887b3496cc80fe21059731ad3c084` | 95.125 | Material manual review |
| 50 | 2026-06-21 21:29:43 +0200 | `1eb80cd9b293ab1d810d2c67dcfb033a1fa1056a` | 94.618 | Architecture review |
| 52 | 2026-06-21 19:21:28 +0200 | `6dc25d910382820569ac16e888d5d82be130cbc3` | 96.090 | Parallel implementation of the raytracer |
| 58 | 2026-06-21 13:47:56 +0200 | `78d5803e53119d16884bb5ffd3fe7606068fc624` | 95.171 | Fixing memory leaks, part 9 |
| 62 | 2026-06-21 11:05:41 +0200 | `5d637050db139013c57319bfd4bdfec3f439eb8e` | 94.070 | Fixing memory leaks, part 5 |
| 66 | 2026-06-21 07:43:20 +0200 | `1d5ce6ec071a17a833d52a8da7fa4a048ea283ad` | 94.413 | Fixing memory leaks |
| 71 | 2026-06-20 21:19:39 +0200 | `4300cf31ea01439a8039f11efde4f3dc2955ce6a` | 94.239 | Patch classes revisited |
| 75 | 2026-06-20 20:08:02 +0200 | `b4032f7845ebe25582c58f8831a10b864ac1d265` | 95.320 | PovRayMaterialBuilder revisited |
| 77 | 2026-06-20 18:06:39 +0200 | `00a0228a986d6b9a067471fdf813c49bfb16091d` | 95.578 | Preparation steps prior to implementing concurrent renderer |
| 80 | 2026-06-20 17:09:11 +0200 | `9b3d8b5a1d9bc7f9942b7a0ffcf53efeeec4619d` | 95.629 | PovRayMaterial with no setters |
| 82 | 2026-06-20 13:22:43 +0200 | `a97992c5186cbd1f21831fcc27ed7a2722f9152e` | 94.310 | Manual cleanups and updates, part 2 |
| 86 | 2026-06-20 10:36:55 +0200 | `a3c3609245f6b4cfbd2fb488796640cf0f993641` | 94.226 | Decoupling pov material parser from scene |
| 88 | 2026-06-20 09:34:01 +0200 | `c7afb78e37377407ec29348feaa2667d4b357b69` | 93.645 | RenderWorker class |
| 91 | 2026-06-20 04:36:12 +0200 | `4ef28181b468cffdce2ccb7f005f021b4f0cbd4a` | 95.360 | Cleaning unused branches, deadcode and preparing for multi threads |
| 93 | 2026-06-20 03:15:51 +0200 | `1005e52ca82f99a6678e171f4c4f028ca4ca44bf` | 96.089 | BooleanSetOperations enum revisited |
| 96 | 2026-06-20 02:07:10 +0200 | `79ff259c4785640137be0410b8a76edc0fef84b9` | 93.950 | Code modernization: merge declarations and initializations |
| 98 | 2026-06-20 01:19:20 +0200 | `323d3e1bb8a9708ae56587ea2fda4cfbc3b2d189` | 93.634 | Geometry class hierarchy uses C++ copy constructor |
| 101 | 2026-06-19 23:49:17 +0200 | `3f7b58eaef0721af4d793a6cd48d7e90b9b4b1bc` | 94.122 | Architecture revisited |
| 103 | 2026-06-19 22:54:51 +0200 | `8b4c30566868ce4acf46365b19b6a6ffd9bf7d93` | 93.796 | Replacing globals and static methods for thread safe parameters, part 7 |
| 107 | 2026-06-19 17:48:17 +0200 | `e5f0ed9dcc2fd7c43c257890d7b11410e80c48a1` | 88.402 | Replacing globals and static methods for thread safe parameters, part 3 |
| 112 | 2026-06-19 13:29:07 +0200 | `dd4184bbda5f86ae24dfe28ab6c254dadf4f47c1` | 86.706 | Making classes immutable, part 8 |
| 114 | 2026-06-19 09:22:05 +0200 | `5315cc1b8371a6d3d19b578f087c0edea3803319` | 86.185 | Making classes immutable, part 6 |
| 118 | 2026-06-18 21:14:03 +0200 | `93fafca75aa40c19d4e61d38568a7df8bbc0ec12` | 86.546 | Making classes immutable, part 3 |
| 120 | 2026-06-18 19:22:44 +0200 | `59a14d0c5f1ba05fba3bba216bffc84fca130d8a` | 86.815 | Making classes immutable |
| 125 | 2026-06-18 08:13:21 +0200 | `4746de785140aa1f517494c76cb51431a88033d1` | 86.130 | Using getters and setters in other classes, part 3 |
| 132 | 2026-06-17 23:29:56 +0200 | `67c7116623f83b1b5796b786a0afa3a5d57712e2` | 85.508 | Using getters and setters in BoundedGeometry |
| 134 | 2026-06-17 23:12:48 +0200 | `3e0d22c05576f4b63ce4abf8e093654d546c8ffa` | 85.727 | Using getters and setters in Light and Intersection |
| 137 | 2026-06-17 19:47:10 +0200 | `2ae8803f303e922c89481d6af54db791b1712927` | 86.085 | Vitral sync |
| 139 | 2026-06-17 18:07:27 +0200 | `2a6cad154762834781341daeb376bd9bed722915` | 85.470 | Code cleanup |
| 142 | 2026-06-17 14:29:45 +0200 | `bf00fbf11d799cf32c15b10404df0a3d5612ed9d` | 85.151 | Light decoupled from PovrayMaterial |
| 144 | 2026-06-17 09:06:31 +0200 | `ee39dbd29f9a0486728671d317d23951e1162799` | 85.840 | Performance improvements (performance hit) |
| 147 | 2026-06-16 23:13:40 +0200 | `4ad45464b9d1f8ff94c2557265321c6739958666` | 92.350 | Warnings in Mac cleared |
| 149 | 2026-06-16 21:32:09 +0200 | `2d3ea3dd587edcda72159dbd9e0cad9dd43bc248` | 92.321 | Performance update |
| 152 | 2026-06-16 00:19:23 +0200 | `91f4e7a5deeb505d452abb7e7761026da6b28de6` | 75.933 | Geometry decoupling, part 1 |
| 154 | 2026-06-15 21:13:02 +0200 | `1bfff6ff8e57aa75f0d4f8b41696ff6786189db7` | 76.020 | Environment names revisited |
| 162 | 2026-06-13 05:14:04 +0200 | `4494605b2d012570974c24af1c843e23af2bd1e1` | 59.838 | Polynomial solvers design revisited |
| 164 | 2026-06-13 04:27:52 +0200 | `c5e0f12d1a1e26ffd59fb08cf6b524ad0522cb60` | 59.973 | Polinomial solver revisited |
| 166 | 2026-06-12 23:51:26 +0200 | `a5abd2942bf8c9f6de5d140dc9f920fb46844ea8` | 59.705 | Solid textures moved to vitral library |
| 168 | 2026-06-12 22:33:03 +0200 | `da3584a9e1ee5835320dacc85ef934f0fd1b39f7` | 58.736 | Solid textures package ready to be moved |
| 171 | 2026-06-12 18:51:34 +0200 | `6026914fe702be2213067aaa3c7e78f9abb9667f` | 59.260 | Making symbols const when possible, part 2 |
| 173 | 2026-06-11 22:38:40 +0200 | `8575a1056a15270681b9d4b43d86efdce36dc3c9` | 59.703 | Modernized comments |
| 178 | 2026-06-11 17:55:33 +0200 | `5bcbb445d14a5b6b3c72815d4813bd083218a152` | 59.119 | Architectural review, cleaning solidTexture package to move it out to shared library |
| 182 | 2026-06-11 12:02:43 +0200 | `1952ffc61cd4141a2bde32e1b2135141ae5fe309` | 59.627 | Last free functions are now methods, start using consts |
| 187 | 2026-06-10 23:03:57 +0200 | `507072c369b3ad1a29d88b9f1177e443039fb3c6` | 59.510 | Decoupling Material from FixturesFacade |
| 189 | 2026-06-10 20:56:18 +0200 | `3611ae1623101d5e62431092d29d86cce1d89c81` | 59.577 | Revert "Removed CHECKER_TEXTURE_TEXTURE" |
| 194 | 2026-06-10 14:45:52 +0200 | `2b6b35013ab3b3bb3e73103971105835140bb149` | 59.053 | Decoupling solid textures from material model, part 3 |
| 196 | 2026-06-10 12:05:09 +0200 | `eeb0f0e6018462ab55092a0c92d45c48b40176e4` | 58.865 | Decoupling solid textures from material model |
| 198 | 2026-06-10 08:48:36 +0200 | `a5b3c83e20111838dbee7f9d23abb0b94e94c0f2` | 58.957 | Solid texture is a main package |
| 200 | 2026-06-10 08:01:09 +0200 | `241be1e29856be95207de3751b929c78081fb6b8` | 58.832 | Includes in canonical order |
| 205 | 2026-06-09 22:47:57 +0200 | `bf5db0cfb54de0dac6cd66f08b9bc9dfe111d361` | 59.822 | PovColorMap to color |
| 207 | 2026-06-09 20:44:10 +0200 | `6fe06f65d147fad504ed97c249f8da7408e4588c` | 59.520 | ColorOperations revisited |
| 209 | 2026-06-09 19:35:36 +0200 | `01f950ffbaf7336dc16f077c19a0e03bd7c36aea` | 59.284 | ColorRgba move to library layer |
| 213 | 2026-06-09 18:01:21 +0200 | `6090efef8a53b307877bfe7578c761854b6eba18` | 58.774 | Solid texture architecture revisited |
| 218 | 2026-06-09 11:39:30 +0200 | `ed705bce7e75cdea3cbff3ea4b93875ec75e02b0` | 59.483 | Performance bugfix in using Matrix4x4d transforms |
| 220 | 2026-06-09 07:41:56 +0200 | `db09ed478a3083a73b04cc18ee8ab600f1179ab3` | 106.175 | Logger revisited |
| 224 | 2026-06-08 23:27:01 +0200 | `fd21e63831c5d17348e67388af2369322e697e5a` | 104.824 | Small refactors |
| 228 | 2026-06-08 18:56:10 +0200 | `8c3cee6b14b17c670d38cd362badb3cbccabb9dc` | 58.529 | Inlined Vector3Dd.h gets double performance |
| 230 | 2026-06-07 20:34:26 +0200 | `16ba597ce3bb1a6d52f33b8c1a4cf50eb89a5d8b` | 57.479 | Using vitral dependencies |
| 232 | 2026-06-07 14:24:30 +0200 | `28424989ff9c21dec1079005c29a7816140db74e` | 57.342 | Io layer refactored |
| 236 | 2026-06-07 01:10:37 +0200 | `de85837eed3ab98478ebb5008a08aeb2e23c71bd` | 56.898 | Architecture: simplifying #includes |
| 242 | 2026-05-26 08:07:15 +0200 | `723e23afe1b191617ba08524fdc6744580a47066` | 57.306 | Antlr POV parser (part 2) |
| 246 | 2026-05-26 00:44:40 +0200 | `ba2ecae21d3cb673d34d95e9cf78e72aa38e79ae` | 57.450 | Pov parser reorganization (part 31) |
| 248 | 2026-05-26 00:28:12 +0200 | `7b18899fc368baa5e55366b57dc9077658f1b6a2` | 57.076 | Pov parser reorganization (part 29) |
| 253 | 2026-05-25 21:57:54 +0200 | `79b32e1e8f4e5e677c50f1b5de49fd12374d8100` | 57.124 | Pov parser reorganization (part 24) |
| 257 | 2026-05-25 00:05:58 +0200 | `8ece10df7e7d847ceda5a3ca150469bb5e9228df` | 57.099 | Pov parser reorganization (part 20) |
| 259 | 2026-05-24 19:14:36 +0200 | `bcb67853df76de9e1fff2b9a1997b1452be3de95` | 57.105 | Pov parser reorganization (part 18) |
| 263 | 2026-05-24 17:43:26 +0200 | `a6fc69d76779ec748b4ed87a5c9df695608054f7` | 57.122 | Pov parser reorganization (part 15) |
| 266 | 2026-05-24 16:32:03 +0200 | `aa65bb7286bb0bef7962647b2c3ad3d837f887e0` | 57.416 | Pov parser reorganization (part 12) |
| 269 | 2026-05-24 15:15:35 +0200 | `01a397f1c1e9e93110928385301a516eefed901e` | 58.054 | Pov parser reorganization (part 9) |
| 273 | 2026-05-24 14:43:43 +0200 | `8d501e407fcf6eca91d370ca149cae9639fd5681` | 56.937 | Pov parser reorganization (part 5) |
| 276 | 2026-05-24 14:07:40 +0200 | `20e5a47fbf4575309f75f2c469284490920354af` | 57.851 | Pov parser reorganization (part 2) |
| 279 | 2026-05-24 10:54:53 +0200 | `4b976a45d8823acc8b60021d152e4be03d506722` | 58.064 | Replacing global functions and variables by class methods and attributes (part 3) |
| 281 | 2026-05-24 10:06:51 +0200 | `7bececc6b66930979afc319518afd533b589d077` | 57.705 | Replacing global functions and variables by class methods and attributes (part 1) |
| 283 | 2026-05-23 23:27:15 +0200 | `1ceeac94b97b48ecbf1e72e9b0d47a6293eed26b` | 57.611 | Architecture revisited |
| 288 | 2026-05-23 19:54:03 +0200 | `3c2ca93dc5ce8ecc0a92217f3b5036fc91383911` | 54.249 | Code format, attributes names |
| 290 | 2026-05-23 19:21:22 +0200 | `3bfb1cd507a1cc41c8eae1739ede06e78bc92641` | 55.182 | Shaders separated and explicit |
| 294 | 2026-05-23 17:03:17 +0200 | `57257fe59fc70a7e9b9f11f8b9f72575f6070001` | 54.364 | Statistics decoupled |
| 298 | 2026-05-23 16:42:40 +0200 | `b592d7133ed576c1d4e4ad535041a5fd220eed52` | 54.780 | Using Logger for info messages |
| 301 | 2026-05-23 15:45:32 +0200 | `3daaeeac1a82592cbc6b5fb6803bf187dcb3ad9b` | 54.758 | Architecture revisited |
| 304 | 2026-05-23 13:59:07 +0200 | `c52a90b7ab8d79f6041551351347b1ea818fbe20` | 54.374 | I/O classes organized |
| 308 | 2026-05-23 13:13:48 +0200 | `8531eb67a3107e849f59c6809ebaca65d2474c16` | 55.426 | Decoupling geometry and scene |
| 310 | 2026-05-23 12:41:37 +0200 | `8516bc6175b7ab29ac3a7df69219867d61337640` | 54.855 | Logger class |
| 315 | 2026-05-17 19:52:10 +0200 | `da5bffababa2941440aa3d357546dbc394a4f957` | 61.377 | Build warnings removed for MacOS |
| 317 | 2026-05-17 15:47:01 +0200 | `50d3d5b8694519da218af9ddbac42d36320e02e4` | 24.700 | Expressing extended parts of Ray in a separate class |
| 321 | 2026-05-17 12:54:14 +0200 | `e4f61ed5a65f99827afd2d3c8526e534cac28c29` | 24.556 | Normalized Vector3Dd |
| 325 | 2026-05-17 11:54:42 +0200 | `08c4e8135858b31c38fe42d3813eca339302afd5` | 24.257 | Empty modules removed |
| 327 | 2026-05-17 11:10:20 +0200 | `d1101703866aa1975d24b6ff66cc7192e1a5a031` | 24.495 | Revisiting class names and modules |
| 329 | 2026-05-17 10:31:54 +0200 | `a59af44775faa8160d3d95181a229e48d0920b27` | 24.541 | Small pov parsers |
| 333 | 2026-05-17 09:52:22 +0200 | `7df7f49685cd57d4a45cd4a1d2c5f96c17695f04` | 24.077 | Each class in its own module |
| 336 | 2026-05-17 05:38:01 +0200 | `84d5b98d6d84ec79d8cfdffaffd5d404d958d23b` | 24.683 | Putting functions inside classes as methods, part 6 |
| 339 | 2026-05-17 02:56:07 +0200 | `ab777f5e70089476c8006ef3a26f7d7c7f17973c` | 24.868 | Using double in all modules |
