# Geometry/scene convergence execution record

This record accompanies [the plan](vitralPlan.md). It records evidence and open
work without turning the forward-looking plan into a history of completed stages.
No implementation is considered migrated merely because its file has a destination.

## Stage A: inventory and acceptance specification

The machine-readable [inventory](vitralConvergenceInventory.json) covers all 69
files currently under `raycasting`, including build definitions and tests.
Run `python3 scripts/auditVitralConvergence.py` to check coverage and the local
`base/src/main` mirror. Use `--vitral-root` for a different VITRAL checkout.
A `split` action means moving disjoint responsibilities to their owners, never
maintaining copies of the same mathematical implementation.

### Parser inventory

The entry points are `src/io/pov/scene/ObjectParser.cpp`,
`src/io/pov/geometry/CsgParser.cpp`, and the primitive parsers in that directory.
Declarations reuse these constructions through `SceneParser` and the symbol table.

| Accepted construction | Current mathematical representation | Final responsibility | Acceptance |
|---|---|---|---|
| `sphere` | `RayCastingSphere`, shared `Sphere::intersectionRoots` | One VITRAL `Sphere`; remove the second radius/inversion object | G1 |
| `box` | `RayCastingBox` | One VITRAL `Box` | G1 |
| `plane` | `RayCastingInfinitePlane` | `InfinitePlane` surface / `HalfSpace` solid semantics | G2 |
| `triangle`, `smooth_triangle` | `RayCastingTriangleMesh` | VITRAL mesh data, intersection and optional interpolated normals | G2 |
| `quadric` | `Quadric` | Generic coefficients, sign domain and bounds in VITRAL | P1 |
| `cubic`, `quartic`, `poly` | `PolynomialShape` | One implicit polynomial geometry, degree and solver strategy | P1 |
| `blob` | `Blob`, components and interval data | Implicit field, threshold, roots and gradient in VITRAL | P1 |
| `height_field` | `HeightField`, blocks and traversal state | Decoded height samples and geometric domain in VITRAL; file decoding stays in IO | P1 |
| `bicubic_patch` | `ParametricPatch` / patch solver | Merge with VITRAL `ParametricBiCubicPatch` and its intersector | P1 |
| `union`, `intersection`, `difference`, `inverse` | CSG operands and two evaluator subclasses | One expression and separate geometric evaluator strategies | C1 |
| `object`, `composite`, declarations/instances | `RayCastingBody`, `RayCastingComposite`, builders | Shared placement/grouping; POV binding and syntax policy outside geometry | S1 |
| `bounded_by`, `clipped_by` | Body/composite bounds and clips | Generic query operations; parser preserves language semantics | B1, S1 |
| `light_source`, texture/color/`no_shadow` | Light adapter and body attribution | Scene/appearance, not a new mathematical primitive | S1 |

`poly` accepts degrees 2 through `PolynomialSolver::MAX_ORDER`; migration must
retain that parser constraint rather than assuming that only quartics exist.
VITRAL-only primitives remain supported; the above table is the POV input catalog,
not a replacement list for VITRAL geometry.

### Consumers and duplicated responsibilities

| Consumer | Required change | Acceptance |
|---|---|---|
| `ShadersModel`, `SoftwareRaycaster`, `OpenGlOfflineSphereRenderer`, OpenGL sphere renderer | Share the sphere definition; preserve software/OpenGL, texture, bump, tessellation controls; later expose other geometry fixtures | G1, D1, V1 |
| MIT reader, `SimpleBody`, `SimpleSceneSnapshot`, `SimpleRaytracer`, `ParallelRaytracer` | Keep MIT scenes and both executors; adopt shared queries and instance placement | S1, V1 |
| POV primitive parsers and `SimpleBodyBuilder` | Construct VITRAL objects directly; keep defaults, syntax, materials and transform history in POV | S1, U1 |
| POV shaders and `RayWithTracingState` | Keep media, IOR, ray category and appearance state outside mathematical rays; extract reusable mathematical workspace | N1, S1 |
| `BakedGeometryBaker`, `BakedQuadricIntersector`, `BakedPlaneIntersector`, `BakedTrace` | Share mathematical kernels; retain renderer-owned layout/scheduling and derived immutable data | G1, P1, V1 |
| `CsgContainmentTest`, `CsgOperandTrace`, `SingleCorePlaneCsgTrace`, `CsgMorganUnionTrace`, `RaySegmentCsgCombiner`, `CsgFirstHitTrace` | Audit optimized CSG paths alongside object evaluators; prevent a second independent CSG implementation | C1, V1 |
| `SurfaceRayIntersection` and deferred detail callbacks | Preserve leaf and instance identity, coordinates and material lookup with one geometric detail vocabulary | D1, S1 |

### Acceptance corpus specification

These IDs link each migration entry to concrete acceptance cases. They are
requirements for executable tests, not claims that the current code passes them.
Existing integration tests are starting evidence, not substitutes for this corpus.
The initial machine-readable analytic cases are in
[`tests/fixtures/vitralGeometryContracts.json`](../tests/fixtures/vitralGeometryContracts.json):
six sphere queries and four interval combinations with boundary attribution.
The sphere cases now have corresponding native API tests in stage B. Interval
combination fixtures remain acceptance data for stage E; merely parsing this file
does not validate a Boolean evaluator.

| ID | Required cases and independently checkable expectations |
|---|---|
| G1 | Unit sphere, ray `o=(-3,0,0), d=(2,0,0)`: crossings `t=1,2`, IN between them. Interior start `o=(0,0,0), d=(2,0,0)`: exit `t=0.5`, initial IN. Tangent `o=(-2,1,0), d=(1,0,0)`: surface event at `t=2`, no solid state change. Clipped `[1.25,1.75]`: no crossings, entire segment IN. Repeat under nonuniform/negative scale and shear while preserving affine `t`. Nearest admissible event must agree with event enumeration under the same policy. |
| G2 | Plane `x=0` vs half-space `x<=0`, parallel/coplanar rays, origin on boundary, positive/negative direction. An open triangle/patch supports surface queries but must explicitly reject volumetric classification; smooth normals must not change membership. |
| C1 | Analytic operand intervals A=`[1,3]`, B=`[2,4]`: union `[1,4]`, intersection `[2,3]`, difference `[1,2]`, with the subtracting boundary reversed. Disjoint, nested, identical and touching operands; initial IN, clipped and unbounded intervals; grouped coincident events and tangencies. Specify ON policy before asserting contacts. Heuristic union may retain internal surfaces; assert that documented behavior separately from Roth membership. |
| D1 | Requested vs available detail; no fabricated UV/tangent data; inverse-transpose normals; nested difference orientation; originating leaf and instance chain survive deferred evaluation. Depth beyond fixed attribution capacity must succeed or report an explicit error, never truncate. |
| N1 | Reject invalid query intervals and zero-direction rays explicitly; distinguish unsupported, empty and indeterminate. Test repeated roots and scaled inputs with separate solver/geometric tolerances. Reuse workspace across queries and workers without stale data or material ownership. POV shadow cutoffs become explicit application query policy. |
| B1 | Empty/finite/unbounded bounds; conservative slabs for parallel rays and interval endpoints; clipping must not lose admissible hits. Infinite planes and complements must not be replaced by finite boxes. |
| P1 | Analytic quadratic/polynomial roots and gradients, repeated quartic roots, blob threshold crossings, heightfield sample interpolation/base/closure, bicubic control points and derivatives. Construct each object with VITRAL materials only. No implicit inside for open surfaces. Compare exact queries with derived display meshes within declared approximation error. |
| S1 | Shared geometry in two differently transformed instances, material binding independence, destruction/clone/snapshot lifetime, cache invalidation. Preserve POV ordered placement and texture policies without storing those policies in geometry. |
| V1 | Full POV render gate with repeated timings and hashes; both CSG modes; all VITRAL examples build; all MIT scenes sequential/parallel; ShadersExample software/OpenGL image fixtures including textured/bumped and rotated sphere. Record graphics failures as limitations, never passes. |
| U1 | No VITRAL dependency on POV or raycasting; exactly one authoritative mathematical implementation, including baked paths. Remove raycasting target/directory and transitional bridges; keep the retained base mirror byte-identical. |

### Contract gaps to resolve in stage B

- Resolved in the affine increment: native sphere first-hit overloads and the
  scene fast path now use `intersectionRoots`, including interior exits. The
  duplicate `RayCastingSphere` object is still a stage C concern.
- `Geometry::emitRayCrossings` distinguishes unsupported from empty, but there is
  no complete solid interval result with an explicit initial membership and
  indeterminate state. A list of hits cannot represent a clipped all-inside ray.
- Resolved in the affine increment: component copies retain parameterization,
  and `Ray::fromAffine` explicitly constructs an unnormalized ray. Legacy
  constructors keep normalization for compatibility.
- `RayCrossingQuery` includes compatibility chord/approach cutoffs. Their role must
  be explicit application policy, rather than an implicit definition of a sphere.
- `RayCrossing` carries a material binding; the future mathematical event must
  carry provenance while scene resolves materials.
- `RayCastingQueryAdapter` constructs renderer configuration/statistics and a
  queue pool per adapter. It must disappear when shared queries are native.
- `RayWithTracingState` combines mathematical caches with media/IOR, primary/shadow
  flags and renderer configuration. These responsibilities need separate owners.
- `GeometryConfig` combines intersection, solver and application distance cutoffs.
- Legacy `getMinMax()` does not express empty/unbounded/lifetime semantics as a
  typed contract. Preserve its consumers while introducing an explicit result.

## Validation evidence

The baseline report is stored separately in `vitralConvergenceBaseline.json`.
Reproduce the example corpus after building VITRAL with:

```sh
python3 scripts/captureVitralConvergenceExamples.py --output /tmp/vitral-convergence-new-examples
```

The output directory must be new. The tool refuses to overwrite references,
checks every process and image, and verifies sequential/parallel equality. OpenGL
requires `xvfb-run` and a working GL implementation. It records backend-specific
shader hashes without claiming software/OpenGL equality.

Raw logs and images are retained under `/tmp/vitral-convergence-baseline` for this
session. Hashes in the report are durable evidence; temporary images must be
archived elsewhere before cleaning `/tmp` if pixel-level investigation is needed.

### Results at the starting revisions

- Both repositories started clean. No runtime source changes or commits were made.
  VITRAL's build cache was configured to include VGA examples; its tracked sources
  remain unchanged.
- All configured VITRAL examples built, including VGA and the optional OCCT tool.
  VITRAL passed 110 enabled tests; the POV build passed 111. Each build has the same
  pre-existing disabled Kurlander bowl topology test, recorded in the JSON report.
- Two full default POV runs: 111.59 s / 112.39 s wall time, 1291.53 s / 1295.18 s
  user CPU. All 108 images are identical between runs and to the historical hashes.
  These measurements establish a baseline, not a post-migration speed claim.
- All 12 MIT scenes rendered in both execution modes with identical per-scene
  hashes. All six shader fixtures rendered successfully, including real OpenGL
  execution under Xvfb. Recapturing all 30 images with the reusable capture tool
  produced identical hashes.
- Roth rendered all 108 scenes successfully in 205 s. Its comparison against the
  general golden images **fails at the existing 0.02 RMSE threshold** for seven
  scenes, before any mathematical implementation changes:

| Scene | Normalized RMSE |
|---|---:|
| `level2/iortest` | 0.0361584 |
| `level2/pawns` | 0.0704312 |
| `level3/pool` | 0.0289496 |
| `level3/snail` | 0.0372661 |
| `level3/takeoff` | 0.0272118 |
| `level3/wg5` | 0.0356993 |
| `math/trough` | 0.052355 |

The comparator also rejected scientific notation through `bc`. It now validates
numeric input and compares it with `awk`; the threshold was not changed. The same
seven scenes fail after the fix, without numeric-parser errors. Synthetic checks
cover small/large scientific values, threshold equality, invalid metrics, and an
invalid threshold. These differences predate implementation. The user has explicitly accepted these
seven scenes as the starting exception set: preserve the normal render flow and
reject any additional Roth discrepancy. This acceptance does not assert that the
two CSG algorithms have identical semantics.
Roth-specific hashes are preserved separately from the general golden references.

Stage A inventory, acceptance specification, and baseline capture are recorded.
The regression gate now accepts the seven recorded Roth exceptions and rejects
new scenes, missing outputs, and comparison errors. Use
`python3 scripts/checkVitralConvergenceRoth.py` after rendering the Roth corpus. Stages B–H are not
complete merely because their acceptance cases or destinations are specified here.
The following iteration begins that mathematical contract; the sphere
first-hit/affine-copy gaps were still pending at that point.


## Stage B: native solid classification (first increment)

Added `Geometry::classifyRay`, a typed status, reusable segment classification,
and mathematical boundary provenance to VITRAL base. `Sphere` supplies the first
implementation through its existing root kernel. Each new class has its own
header/implementation; statuses use an enum, with no new namespace or struct.
See [the contract](vitralGeometryContracts.md) for endpoint, tangency, ownership,
query policy, and numerical semantics.

This increment does not switch the POV render path or eliminate the duplicate
sphere object. It establishes the missing all-inside/all-outside result before
consumer migration. The local base mirror is synchronized to VITRAL (149 files).
VGA remains a base consumer and requires no separate autonomous validation.

The new Roth regression wrapper preserves the recorded seven exceptions without
increasing the RMSE threshold. Its validation explicitly checks that a new scene
or a missing image still fails, while fewer known discrepancies are acceptable.


| Plan stage | Status after this increment | Remaining work |
|---|---|---|
| A. Specification and baselines | Complete | Maintain the inventory and accepted baseline as migrations proceed |
| B. Mathematical contracts | Complete | Contracts and sphere/decomposed-scene reference implementation validated; migrate other primitives and adapters in C–H |
| C. First vertical slice | Not started | Remove the duplicate sphere object and connect all three applications |
| D. Basic primitives and composition | Not started | Consolidate box, plane/half-space, triangles, quadrics and placement |
| E. CSG evaluators | Not started | Consume common classifications in both evaluators and optimized paths |
| F. Complete catalog | Not started | Migrate advanced primitives and expose them in VITRAL examples |
| G. Final scene model and consumers | Not started | Consolidate scene ownership, bindings, snapshots and bake |
| H. Removal and completion | Not started | Remove remaining bridges, duplicates and raycasting |

### Validation of the contract increment

See [the validation report](vitralConvergenceContractsValidation.json) for commands,
timings, test counts and comparison results. All enabled tests pass: 116 in the
VITRAL build and 117 in the POV build, including six new native classification
test groups. The existing disabled topology test remains unchanged.

The normal POV corpus preserves all 108 baseline images. The 12 MIT scenes in
both modes and six shader fixtures preserve all 30 baseline images after the final
rebuild. The new classification query has not yet replaced renderer queries.

Roth retains the same seven accepted exceptions and introduces no additional
scene above the threshold. Its images are not claimed to be byte-identical:
28 hashes differ from the initial capture. Two repeated parallel renders of
`level1/stone1` with the same binary also produce different hashes (normalized
RMSE between repeats: 0.00161563). This establishes execution variability without
attributing every difference to a specific cause. The gate uses the explicitly
accepted exception set, while retaining the hashes for investigation.


### Native Java abstractions follow-up

Replaced the newly introduced STL containers with `java::ArrayList` in solid
classification and its test collector. Class declarations include template `.h`
files; implementations include `.txx`. Classification copy construction,
assignment, and destruction are defined out of line, and list elements have the
default construction required by the native container. Tests verify independent
copies and retained storage across repeated queries.

The test collector now has its own `RayCrossingTestCollector.h/.cpp`, without an
anonymous namespace. Newly added finite/NaN checks and constants use `java::Double`,
with class methods implemented in the existing Double module. No new global
helpers, namespace blocks, or structs are introduced. Phase statuses remain A
complete, B partial, C–H not started.


The Java-abstraction follow-up passes 118 enabled VITRAL tests and 119 POV tests.
All 108 normal images and all 30 example images match the starting baseline;
Roth retains exactly its seven accepted exceptions. Normal-render wall times were
117.41 s and 111.25 s; the repeated run is within the initial wall-time range.
CPU totals remain higher than the original observations, so no claim of a CPU
speedup or exact performance equivalence is made. Both measurements are retained
in [the follow-up validation report](vitralConvergenceJavaValidation.json).


## Stage B: affine copies and shared sphere first-hit kernel

Added `Ray::fromAffine` and made component-copy methods preserve the unchanged
ray components. Legacy constructors still normalize. Both native sphere nearest-hit
entry points and the `SimpleBody` sphere fast path now use the shared root kernel;
translated and scaled scene queries preserve the world parameter in returned
rays and detail. The sphere also exposes an explicit inclusive first-hit interval.
No STL containers, namespace blocks, structs, or global helper functions were
introduced. The new tests use `java::` abstractions and the existing collector
class module.

All 12 MIT scenes in both execution modes remain byte-identical to the initial
references. All three OpenGL fixtures and the plain software fixture also remain
identical. Two software shader fixtures differ:

| Fixture | Changed pixels (320×240) | Maximum channel difference | Normalized RMSE |
|---|---:|---:|---:|
| Default textured/bumped sphere | 67 | 30 | 0.0006610582851028578 |
| Rotated textured/bumped sphere | 2 | 2 | 0.000021615615286475414 |

The changed default pixels are near the shadow terminator and become ambient-only.
The renderer offsets shadow origins toward the light; that can place them inside
the sphere. The new analytic test proves that such a ray must find its positive
exit. This explains a meaningful behavioral consequence of correcting inside
hits, rather than introducing a renderer-specific geometry exception to conceal
it. Small arithmetic differences from the unified kernel can also affect pixel
quantization. The original references have not been overwritten. The images were
inspected, and the measured differences are retained for review.

Phase A remains complete and phase B partial. C–H have not started; in particular,
`RayCastingSphere` has not yet been removed and neither CSG evaluator has been
migrated to the new solid-classification result.

## Stage B: typed bounds (initial implementation)

Added a caller-owned `GeometryBounds` result with explicit unsupported, empty,
finite, unbounded and indeterminate states. Sphere implements local finite bounds;
other geometries explicitly report unsupported until migrated. The existing
`getMinMax()` consumers continue to work. A native test covers the states,
validation, reuse and sphere values. The class has its own `.h/.cpp` and uses no
STL containers, namespace blocks or structs.

The full contract remains partial: bounds still need implementations for the
remaining geometry catalog and integration with consumers; explicit detail
availability, extensible instance/surface provenance, and a generic reusable
workspace are not yet implemented by this increment.

## Stage B: detail availability (initial implementation)

Added a separate known/unknown availability state and mask to `RayHit`; requests
remain independent. The sphere marks only finite, calculated fields. Native tests
cover a pole with unavailable UV/tangent, independent copies and reset. Legacy
producers retain an explicit unknown state until migrated. No STL containers,
namespace blocks, structs or global helpers were added.

This does not complete phase B: detail propagation in other geometries and scene,
extensible provenance, and a general per-query/worker workspace remain pending.
The existing classification result already retains its list capacity for reuse,
but is not claimed to be the complete generic workspace.


Validation: 128 enabled VITRAL tests and 129 POV tests passed; 30 example
images match the preceding iteration. Both normal renders match all 108 baseline
images (117.26 s and 111.61 s); Roth retains the seven accepted exceptions with
no new ones. See [the report](vitralConvergenceDetailValidation.json).

## ShadersExample: microfacet material loading fix

The F7 controller cycles through all five shading types. The shared CPU/GPU
material loader had a dangling pointer: `parseDoubleOr` and `parseIntOr` stored
`trim(s).toCString()` after the temporary `java::String` had been destroyed.
The allocator could overwrite the text before `strtod`/`strtol`, silently
selecting default material parameters. Keeping the trimmed string alive for the
conversion fixes both renderers without adding a new material model or STL
container.

A probe using the actual Copper CSV row changed from fallback roughness 0.35,
alpha 0.1225, and eta `(0.2,0.9,1.1)` to the row's 0.25, 0.0625, and
`(0.27,0.68,1.32)`. Regression tests cover two material rows, floating-point and
integer fields, whitespace, and the full F7 sequence. Offline software/OpenGL
rendering of all five modes and interactive F7/material cycling under Xvfb are
also checked.

A separate limitation predates this fix: CPU Flat and Gouraud instantiate
`LightingShader` with identical arguments, and their captured images are
identical. Correct distinct flat/Gouraud evaluation for ray casting remains
unresolved; the material parsing fix does not claim to implement that behavior.

Validation: both applications were built; 130 VITRAL tests and 131 POV tests
passed, with the pre-existing disabled test unchanged. Interactive Xvfb runs
confirmed F7 and material selection in GPU and CPU, and offline runs covered all
five shader options on both backends. The 108 normal POV images remained identical
(110.75 s); Roth retained the seven accepted exceptions without new ones. Details
are in [the shader validation report](vitralShaderValidation.json).


## Stage B: contract exit audit

Stage B is complete at **contract and reference-implementation level**. This
is not completion of the later geometry, CSG, scene or application migrations.

| Contract | Reference implementation and evidence |
|---|---|
| First admissible hit | `Geometry::queryFirstHit`: typed hit/miss/unsupported/invalid/indeterminate status, inclusive interval, optional geometric boundary and lazy detail; sphere shares its root kernel |
| Surface events | `RayCrossing` uses the same generic `RayBoundary`/provenance model; sphere distinguishes invalid and indeterminate input from an empty supported sequence |
| Solid intervals and containment | Sphere classifies IN/OUT segments, tangency and initial state; typed point containment distinguishes boundary, invalid, indeterminate and unsupported |
| Bounds and detail | Explicit bounds kinds and detail availability, with no inferred legacy capability; sphere supplies the reference implementation |
| Provenance | Borrowed leaf/instance identities, surface locator, orientation and unbounded instance path with copied forward/inverse transforms; decomposed `SimpleBody` appends its instance |
| Workspace | Caller-owned per-worker storage with stable, reusable nested scratch values and checked LIFO release; no STL containers or POV state |
| Numerical and placement policy | Affine `t`, endpoint rules, small and large inputs, singular/non-finite placements, nonuniform tolerance and unsupported surfaces have explicit outcomes |

The native tests cover interior, boundary, tangency, degeneracy, negative and
nonuniform scale, shear parameter preservation, singular/invalid placements,
32-step paths, nested workspaces and concurrent workers without depending on
PovRay. ASan/UBSan run the selected 22 tests. The normal test suites, full example
build, offline and shader fixtures, `renderAll.sh` and the accepted Roth gate all
pass; [the validation report](vitralStageBValidation.json) contains the evidence.

Remaining work is deliberately assigned to later stages: `RayCastingSphere` and
other duplicate geometry families remain; unmigrated primitives and the legacy
POV scene adapter can return `Unsupported`; coincident CSG event policy and the
full Roth/heuristic evaluators remain in F; scene ownership and appearance binding
remain in H; compatibility fields and `raycasting` are removed in I. In particular,
positive world-space tolerance under nonuniform scale is currently explicit
`Indeterminate` and must be addressed by the CSG/scene integration instead of
being silently approximated.

## Stage C: shared sphere state (in progress)

`RayCastingSphere` now owns a VITRAL `Sphere` instead of maintaining a second
radius. Radius changes and copies construct or update the VITRAL geometry, and
the POV containment heuristic evaluates its implicit function. Inversion and
the existing tolerance comparison remain POV policies; in particular, that
comparison is in squared implicit-function units and is not the world-distance
containment contract planned for stage E. The existing root kernel remains
shared by first-hit and crossing paths.

The integration test checks the shared radius, copy independence, native
first-hit and crossing queries, and the legacy boundary and inversion decisions. This is an
intermediate migration, not the stage C exit: the sphere-specific adapter,
its scene and optimized CSG bindings, and its detail callbacks still need to
be replaced by integration of the VITRAL geometry. No claim is made that the
three applications already use one scene representation.

The full VITRAL example build and its 142 enabled tests passed; 143 enabled POV
tests passed, and the 30 captured VITRAL example images matched the preceding
iteration. The normal POV render produced 108 baseline-identical images in
111.03 seconds (1291.03 user CPU seconds), versus baseline runs of 111.59 and
112.39 seconds (1291.53 and 1295.18 user CPU seconds). The existing disabled
test remains disabled. The copied base subset audit reports 161 identical files.
The full Roth render also completed, and the comparison reported exactly the
seven previously accepted discrepancies and no new ones.

### Native leaf provenance and optimized tracing

The transitional geometry bridge now exposes its intrinsic mathematical leaf.
Crossings emitted directly, through a POV body, and from CSG identify that leaf
instead of the bridge; integration tests check the sphere identities, including
both leaves of a difference. The leaf is borrowed and remains valid for the
lifetime of its owning bridge. Other legacy geometries retain their previous
identity until migrated.

The baked CSG dispatcher recognizes the intrinsic `Sphere` and retains a
borrowed pointer in the operand record, so its hot path does not perform a
per-ray dynamic cast or virtual dispatch. `SphereTrace` holds the POV tracing
limits and statistics and calls the VITRAL sphere root kernel, removing the
optimized trace paths' dependency on a sphere-specific geometry adapter.
Inversion, normal detail, ownership and scene binding still require migration;
`RayCastingSphere` therefore remains and stage C is not yet complete.

Validation: 143 enabled POV tests and 142 enabled VITRAL tests passed; the
full VITRAL example build succeeded and the 30 captured example images matched
the preceding iteration. The normal render's 108 images matched the baseline
in 110.46 seconds (1297.86 user CPU seconds), compared with baseline runs of
111.59 and 112.39 seconds (1291.53 and 1295.18 user CPU seconds). The full
Roth render completed with the same seven accepted discrepancies and no new
ones. The inventory audit maps 71 raycasting files and confirms that all 161
files in the local copied base subset match VITRAL.

### Native sphere scene binding

The POV parser now constructs a VITRAL `Sphere` and binds it to the legacy
scene using `NativeGeometryBinding`; the sphere-specific `RayCastingSphere`
source files have been removed. The binding owns its native geometry and
copies it through the optional `Geometry::copyGeometry` contract. Sphere
implements that operation; unsupported geometry copies are explicitly
rejected. The fast sphere traversal delegates to the same VITRAL root kernel,
while POV's query limits, statistics, inversion and historical containment
policy remain in the scene and tracing integration.

The binding can also use the general native first-hit, crossings, containment,
detail and bounds contracts. A non-spherical test fixture exercises those
paths. Unsupported operations and invalid or indeterminate results are not
silently converted into misses or inside/outside answers. The old queue format
cannot represent nested leaf provenance, instance paths or reversed boundaries;
the generic adapter explicitly rejects those events pending the scene work in
stage H. The binding and queue sink remain transitional components to remove
with the rest of `raycasting` in stage I.

Stage C's sphere exit conditions are met: the separate sphere geometry is gone,
the three applications use VITRAL's mathematical sphere, and first-hit and
crossing paths share its root kernel. Both ShadersExample backends and the
offline raytracer ran successfully. This does not complete the later scene,
primitive, tolerance or CSG phases.

Validation: the full VITRAL example build and its 142 enabled tests passed;
143 enabled POV tests passed. All 30 captured example images match the previous
iteration. `renderAll.sh` produced 108 baseline-identical images in 109.90 s
(1305.43 user CPU seconds); baseline runs took 111.59 and 112.39 s (1291.53
and 1295.18 user CPU seconds). Roth completed with the same seven accepted
above-threshold scenes and no new discrepancies. The inventory audit maps 77
raycasting files and confirms 161 copied base files match VITRAL.
