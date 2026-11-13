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
full Roth/heuristic evaluators remain in G; scene ownership and appearance binding
remain in I; compatibility fields and `raycasting` are removed in J. In particular,
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
stage I. The binding and queue sink remain transitional components to remove
with the rest of `raycasting` in stage J.

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

## Stage D: native box contracts (in progress)

VITRAL's centered `Box` now implements the typed first-hit, crossing,
solid-interval, containment, bounds and independent-copy operations. A single
slab computation supplies first-hit, crossing and interval queries while
preserving the affine ray parameter. Containment uses Euclidean distance near
outside edges and corners. A ray overlapping a face and numerically collapsed
or non-finite calculations are reported as indeterminate. The sphere-specific
chord and exterior-approach cutoffs are explicitly unsupported for boxes;
the POV generic scene binding now applies those cutoffs only to the sphere
specialization and can query the VITRAL box using the general contract.

Tests cover crossing order, clipping, endpoint states, tangency, coincident
faces, surface detail, Euclidean tolerance at corners, copying, bounds, and an
instance with negative/nonuniform scale and provenance. Positive world-space
tolerance under nonuniform placement remains explicitly indeterminate for
stage E. The legacy `Box::doContainmentTest` behavior remains unchanged.

Stage D is partial: the POV parser and its optimized renderer still use
`RayCastingBox` and its historical cutoff and boundary policies. Convergence
of that path, plane/half-space, triangle, quadric, shared bounds and the common
CSG model remain to be implemented and verified.

Validation: the complete VITRAL example build succeeded; 146 enabled VITRAL
tests and 147 enabled POV tests passed. All 30 captured example images match
the preceding iteration. `renderAll.sh` produced 108 baseline-identical images
in 110.35 seconds (1296.72 user CPU seconds), compared with baseline runs of
111.59 and 112.39 seconds (1291.53 and 1295.18 user CPU seconds). Roth
retained the seven accepted above-threshold scenes without new discrepancies.
The copied-base audit verified all 161 local files match VITRAL.

### Shared slab kernel for boxes

The native box contracts and POV's `RayCastingBox` now call the same VITRAL
`Box::intersectBounds` slab kernel. It accepts arbitrary local minima and maxima
and a caller-selected parallel threshold. VITRAL's typed queries check arithmetic
and explicitly report overlapping faces and numerical failures; the legacy POV
caller retains its previous threshold, initial interval, distance filtering and
statistics. This also shares the kernel with the bounding boxes used by POV
heightfields. Tests cover non-centered bounds, both ray directions, an interior
origin, parallel misses, and the separate parallel and face-overlap policies.

The intersection kernel is shared, but the complete box geometry is not yet
unified: POV still owns `RayCastingBox`, its bounds, scene binding and historical
containment and normal policies. The VITRAL legacy first-hit path also remains to
be reconciled with its typed query. Stage D therefore remains in progress.

Validation: all VITRAL examples built, with 147 enabled VITRAL tests and 148
enabled POV tests passing. The 30 captured VITRAL example images matched the
previous iteration. `renderAll.sh` produced all 108 baseline-identical images
in 109.99 seconds (1298.14 user CPU seconds), compared with 110.35 seconds
(1296.72 user CPU seconds) in the preceding iteration. Roth kept the same seven
accepted discrepancies without new ones. All 161 files in the copied local
base subset remain identical to VITRAL.

### Infinite plane surface and shared intersection

VITRAL's `InfinitePlane` now provides typed first-hit and crossing queries,
unbounded bounds, independent copying and explicit point/normal detail. The
query keeps the original affine ray parameter and distinguishes a parallel
miss from a coincident or numerically indeterminate ray. It remains an open
surface: solid-interval and point-containment queries are unsupported rather
than assigning a fictitious interior. The generic scene binding can query it.

The general POV plane trace and the optimized baked-plane trace both delegate
their depth calculation to `InfinitePlane::intersectionDepth`. POV retains its
original parallel threshold, hit interval, viewpoint cache and statistics.
Tests cover affine parameters, details, bounds, independent copying, parallel
and coincident cases, numerical underflow, the threshold endpoint and the POV
viewpoint cache. A mathematical half-space, a unique plane/scene representation
and the two CSG evaluator integrations remain subsequent stage D work.

Validation: all VITRAL examples built; 149 enabled VITRAL tests and 150 enabled
POV tests passed. All 30 captured VITRAL example images matched the preceding
iteration. `renderAll.sh` produced 108 baseline-identical images in 111.78 seconds
(1298.86 user CPU seconds), compared with 109.99 seconds (1298.14 user CPU
seconds) in the preceding iteration and the recorded 111.59–112.39 second
baseline. Roth retained the seven accepted discrepancies with no new ones.
All 161 files in the copied local base subset remain identical to VITRAL.

### Planar half-space solid

VITRAL now provides `PlanarHalfSpace`, a concrete solid defined by the negative
side of an `InfinitePlane`. It owns the shared plane representation and uses the
same surface intersection for first hits, oriented crossings and IN/OUT interval
classification. Typed point containment uses local Euclidean distance to the
plane; invalid and numerically indeterminate results remain explicit. Its bounds
are unbounded, and the generic scene binding can query the new solid.

Tests cover both crossing orientations, clipped intervals, endpoint states,
parallel inside/outside rays, coincident boundaries, invalid and underflowing
inputs, independent copying, surface detail, and provenance through a reflected,
nonuniformly scaled instance. This does not resolve positive world-space
tolerance under general affine transforms, which remains stage E work.

The POV parser, legacy half-space policies and optimized plane bindings have not
yet migrated to `PlanarHalfSpace`; both CSG evaluators still require integration
with the common solid model. Stage D remains partial.

Validation: all VITRAL examples built; 153 enabled VITRAL tests and 154 enabled
POV tests passed. The 30 captured example images matched the preceding iteration.
`renderAll.sh` produced all 108 baseline-identical images in 109.67 seconds
(1296.29 user CPU seconds), compared with 111.78 seconds (1298.86 user CPU
seconds) in the preceding iteration. Roth retained the seven accepted
discrepancies and introduced none. All 161 copied local base files match VITRAL.

### Triangle meshes and MIT cow scene

VITRAL's `Triangle` and `TriangleMesh` now share one allocation-free
Möller–Trumbore kernel for their existing first-hit operations, including affine
ray parameters and barycentric coordinates. POV's legacy triangle trace shares
the VITRAL plane-depth kernel, but retains its projected-edge and inversion
policies; it is not yet the same triangle implementation.

The MIT scene reader now imports `triangles` blocks, vertices, polygon faces
and material changes. It triangulates each face as a fan and skips triangles
with repeated vertex indices. Material groups become separate mesh bodies;
their normals are prepared before parallel rendering. The mesh's legacy last-hit
index uses the `java::AtomicLong` abstraction, avoiding a data race on that
field when the same mesh is traced concurrently. The last-hit value remains
shared legacy state; it is not a per-ray provenance mechanism.

Previously `cow.ray` loaded without error but produced a completely uniform
background because its triangle blocks were ignored. The reader test now checks
the six material groups, 3640 triangles and actual ray hits, and the capture
script rejects a cow image without visible mesh-colored pixels. Both sequential
and parallel renders produce the same image and show the cow. `cow.ray` and
`test.ray` now intentionally differ from their earlier incomplete renders;
the other 26 example captures remain identical. Unification with POV's
projected triangle geometry, the complete mesh catalog and shared scene/CSG
models remain pending in stage D and later stages.

Validation: all VITRAL examples built; 155 enabled VITRAL tests and 156 enabled
POV tests passed. The six cow material groups contain 3640 triangles; the cow
render has 423 colors and matching sequential/parallel SHA-256
`098976c03d2baef1bbf0b2b8bc34bbcd5716a773ba28f2298535251952443ab7`.
Among 30 captured example images, only the sequential and parallel `cow.ray`
and `test.ray` renders changed, because their previously ignored triangles now
render. `renderAll.sh` produced all 108 baseline-identical images in 110.01
seconds (1282.60 user CPU seconds), compared with 109.67 seconds (1296.29 user
CPU seconds) in the preceding iteration. Roth retained the seven accepted
discrepancies without new ones. All 161 copied local base files match VITRAL.

### One triangle geometry for both applications

POV's triangle parser and grouped triangle CSG now construct VITRAL's
`TriangleMesh`; the separate `RayCastingTriangleMesh` geometry has been removed.
The native mesh provides typed first-hit and ordered crossing queries, bounds,
independent mathematical copies and deferred surface detail. A face index in
`RayBoundary` carries the identity needed to recover normals and texture
coordinates without relying on the mesh's legacy shared last-hit field.
Mathematical copies do not retain scene material or texture references.

VITRAL's `Triangle` supplies both the common Möller–Trumbore intersection
kernel and the projected-edge mathematics. POV's projected traversal remains
an application policy over the same mesh: it maintains derived per-face
projection and viewpoint caches, the historical inversion and hit policies,
and statistics. It can also bind an existing valid VITRAL mesh without
rewriting its indices or normals. The mesh must only be changed through the
traversal while its caches are attached. Geometry copies rebuild or copy the
relevant policy data without retaining a stale viewpoint cache.

The tests cover triangle orientation and projections, empty and malformed
meshes, affine ray parameters, ordered events and face provenance, deferred
normals and UVs, inversion, copying, and grouped POV triangles. The real MIT
`cow.ray` mesh is also queried and rendered sequentially and in parallel.
Coplanar and numerically ambiguous queries remain explicitly indeterminate;
unsupported nested provenance is not silently discarded. These are conservative
contract outcomes, not an implementation of the later general CSG model.

The triangle geometry and its integration in the current consumers are complete
for this stage. Stage D as a whole remains partial: the other primitive bindings,
placement and bounds still need convergence, the quadric remains to be unified,
and the common CSG composition and the two evaluator integrations remain open.

Validation: all VITRAL examples built; 159 enabled VITRAL tests and 160 enabled
POV tests passed. The 30 example captures, including sequential and parallel
`cow.ray`, matched the preceding iteration. All ten checked ShadersExample
images (five shading modes on both software and OpenGL) matched their reference.
`renderAll.sh` produced all 108 baseline-identical images in 110.83 seconds
(1306.61 user CPU seconds), compared with 110.01 seconds (1282.60 user CPU
seconds) in the preceding iteration and the recorded 111.59–112.39 second
baseline. Roth rendered all 108 scenes and retained exactly the seven accepted
RMSE discrepancies. All 161 copied local base files match VITRAL.

### Planar half-space ownership and box operations

The POV plane traversal now owns a VITRAL `PlanarHalfSpace`, whose boundary
`InfinitePlane` is the sole storage for its mathematical coefficients. The
existing POV type remains a traversal and scene adapter, retaining the primary
ray cache, historical tolerance, statistics and baked-plane integration. The
boundary plane exposes its raw equation coefficients without normalizing them;
this preserves the scale of POV equations. Editing or inverting the equation
invalidates the adapter's primary-ray cache. Tests cover non-unit coefficients,
classification via the native half-space, inversion, copying and cache refresh.

VITRAL `Box` now provides operations for containment and face normals on
arbitrary axis-aligned bounds. The existing POV box delegates these calculations
as well as the slab intersection to VITRAL; the historical boundary inclusion,
face precedence and fallback remain POV policies. Tests exercise non-centred
bounds, boundary inclusion, face precedence and inversion.

This is not yet complete convergence of these primitives: POV still stores its
own box bounds and uses a separate box scene adapter, including heightfield and
baked-scene dependencies. The plane/half-space adapter and the optimized scene
and CSG paths also still need to consume the common scene and query model.
Consequently stage D and this family of primitives remain in progress.

Validation: all VITRAL examples built; 159 enabled VITRAL tests and 160 enabled
POV tests passed. The 30 captured VITRAL examples and the ten checked
ShadersExample CPU/OpenGL shading images matched the preceding references.
`renderAll.sh` rendered 108 baseline-identical images in 113.66 seconds
(1302.54 user CPU seconds), compared with 110.83 seconds (1306.61 user CPU
seconds) in the preceding iteration. Roth rendered 108 scenes and retained
exactly the seven accepted discrepancies. All 161 copied local base files match
VITRAL.

### Shared affine bounds and placement query

`GeometryBounds::transformed` now transforms finite local bounds through the
shared VITRAL affine matrix convention, enclosing the eight transformed corners.
It preserves empty and unbounded states and returns an explicit indeterminate
result for invalid, non-affine or numerically non-finite transformations.
`SimpleBody` exposes `getAffinePlacement` and `getWorldBounds`, so a scene
consumer can obtain placement and bounds without depending on a render policy.

POV's bounding-box conversion now delegates the corner transformation to that
same VITRAL operation. POV stores its legacy matrices in the opposite vector
convention, so its adapter transposes them at the boundary; the shared placement
query composes geometry and object layers in the correct order. Unknown or
invalid bounds fall back to an unbounded POV acceleration box. In the typed
query, complemented native geometries and projected triangle policies report
unbounded bounds conservatively. A POV composite reports unsupported rather than
incorrectly reporting empty when only its children contain geometry.

Tests cover translation, reflection, shear, two placement layers, finite and
non-finite transforms, empty and unbounded bounds, inversion and agreement with
the existing POV culling bounds. The substage is still partial: the common
placement is currently a query over the existing representations, not yet the
one authoritative affine state for both scene implementations; aggregate bounds
for POV composites and the remaining primitive/CSG bindings are not unified.

Validation: all VITRAL examples built; 161 enabled VITRAL tests and 162 enabled
POV tests passed. The 30 captured VITRAL examples and ten checked ShadersExample
CPU/OpenGL images matched their preceding references. `renderAll.sh` produced
all 108 baseline-identical images in 113.40 seconds (1301.20 user CPU seconds),
compared with 113.66 seconds (1302.54 user CPU seconds) in the preceding
iteration. Roth retained exactly the seven accepted discrepancies. All 161 copied
local base files match VITRAL.

### Bounds for composed scene bodies

VITRAL's `GeometryBounds::enclosing` now combines bounds with an empty identity
and explicit finite, unbounded, unsupported and indeterminate outcomes. POV
composites use that operation to enclose their children in composite-local
coordinates and then apply the composite's outer affine placement. Nested
composites, unknown children and unbounded children are covered by tests. The
aggregation deliberately ignores clipping and bounding regions, yielding a
conservative enclosure rather than assuming that those regions can always be
used to tighten it.

The POV box and planar half-space adapters now expose their bounds through the
same typed contract. Inverted boxes are unbounded; invalid bounds and plane
equations retain the native indeterminate result. This makes these primitives
usable by the common placement/bounds query even before their remaining scene
and geometry adapters are eliminated.

The shared bounds query and affine transformation now cover nested composites
and the currently supported primitives. Full authoritative affine state and
bounds for the remaining legacy geometry and CSG representations still require
later scene and primitive convergence; the stage D exit condition is not yet met.

Validation: all VITRAL examples built; 162 enabled VITRAL tests and 163 enabled
POV tests passed. All 30 captured VITRAL example images and ten checked
ShadersExample CPU/OpenGL images matched their references. `renderAll.sh`
produced 108 baseline-identical images in 113.67 seconds (1302.40 user CPU
seconds), compared with 113.40 seconds (1301.20 user CPU seconds) in the
preceding iteration. Roth retained exactly the seven accepted discrepancies.
All 161 copied local base files match VITRAL.

### CSG bounds and planar half-space queries

POV's transitional CSG representation now provides typed bounds by enclosing
its operands' own typed bounds after applying each operand's placement in the
shared affine convention. The enclosure is conservative for union,
intersection and difference; supported complemented children report unbounded
bounds, while unsupported or indeterminate operand bounds remain explicit. Empty CSG
objects currently report unsupported because their operation-specific semantics
are not defined by this adapter. This query does not replace the existing baked
culling policy.

The POV plane adapter now delegates typed point classification, first hit,
crossings and solid interval classification to its owned VITRAL
`PlanarHalfSpace`. Typed events and boundaries identify that native half-space as
their geometric source. Its mathematical copy is likewise a VITRAL half-space.
POV's original rendering, tolerance and caching policies remain in place.
Tests cover native provenance, solid intervals, transformed CSG operands,
difference and complemented operands.

These changes extend the common query but do not complete stage D: POV's box
still has its own geometric storage and historical scene adapter, the common
affine placement remains a query over separate states, and not every geometry
or CSG composition has a unified implementation.

Validation: all VITRAL examples built; 162 enabled VITRAL tests and 163 enabled
POV tests passed. The 30 captured examples and ten checked ShadersExample
CPU/OpenGL shading images matched their references. `renderAll.sh` produced
108 baseline-identical images in 113.51 seconds (1294.70 user CPU seconds),
compared with 113.67 seconds (1302.40 user CPU seconds) in the preceding
iteration. Roth retained the same seven accepted discrepancies without new
ones. All 161 copied local base files match VITRAL.

### Initial VITRAL quadrics and offline fixtures

VITRAL now has `QuadricSurface`, defined by ten quadratic, mixed, linear and
constant coefficients. Its ray equations use the same quadratic/linear root
solver adopted from POV's quadric implementation; POV's existing optimized
traversal calls that solver. The VITRAL geometry supports independent copying,
first hits, ordered crossings, normals, zero-tolerance point classification and
conservative unbounded bounds. Degenerate coincident rays are explicitly
indeterminate. Positive-tolerance point classification and full solid interval
classification remain unsupported until their numerical and CSG semantics are
implemented; the legacy POV wrapper still owns separate coefficients and its
render-specific caches. This is an initial integration, not yet a unique
quadric representation.

The MIT reader accepts `quadric xx yy zz xy xz yz x y z constant`. New
`quadric_ellipsoid.ray` and `quadric_paraboloid.ray` fixtures exercise the
quadratic and linear ray cases in the offline renderer. The capture script
checks that the colored surfaces are visible as well as checking matching
sequential and parallel output. The existing `math` examples include polynomial
surfaces of degree greater than two; those require the later quartic/polynomial
catalog work and are not represented as quadrics.

Validation: all VITRAL examples built; 163 enabled VITRAL tests and 164 enabled
POV tests passed. The 30 existing example captures and ten checked ShadersExample
CPU/OpenGL shading images matched their references. Both new quadric fixtures
rendered visibly and matched between sequential and parallel execution.
`renderAll.sh` produced 108 baseline-identical images in 115.07 seconds
(1305.42 user CPU seconds), compared with 113.51 seconds (1294.70 user CPU
seconds) in the preceding iteration. Roth retained the seven accepted
RMSE discrepancies without new ones. All 161 copied local base files match VITRAL.

### Shared quadric geometry and solid intervals

POV's quadric adapter now owns the VITRAL `QuadricSurface` as its sole storage
for the ten mathematical coefficients. Both consumers use the same implicit
function, gradient and quadratic/linear solver; POV retains only its tracing
policy, viewpoint cache and existing bounding optimization. Its typed first hit,
crossings, point classification, solid intervals and deferred detail delegate
to the VITRAL object, whose identity is carried as boundary provenance.

`QuadricSurface::classifyRay` provides ordered solid boundaries and segment
membership for quadratic and linear rays, complements, clipped intervals and
tangencies. Coincident rays, arithmetic failures and unsupported query policies
remain explicit, with no partial solid result. Deferred normals respect reversed
boundary orientation. Tests cover ellipsoids, transformed instances, provenance,
paraboloids, complemented solids, endpoints and tangencies. General positive
world-space containment tolerance remains stage E work; it is not approximated
using the value of the implicit polynomial.

Three further offline examples show a cylinder, hyperboloid and saddle, in
addition to the elipsoid and paraboloid. Their sequential and parallel renders
are checked for visible surfaces. Quadric bounds remain conservative and
unbounded in the shared mathematical contract; the existing POV finite-bound
optimization is still application-specific. Other phase D scene and CSG
convergence work remains open.

Validation: all VITRAL examples built; 166 enabled VITRAL tests and 167 enabled
POV tests passed. The 34 previously captured example images and ten checked
ShadersExample CPU/OpenGL images matched their references; the three additional
quadric fixtures rendered visibly and identically in sequential and parallel
modes. `renderAll.sh` produced 108 baseline-identical images in 113.49 seconds
(1295.04 user CPU seconds), compared with 115.07 seconds (1305.42 user CPU
seconds) in the preceding iteration. Roth retained exactly the seven accepted
RMSE discrepancies. All 161 copied local base files match VITRAL.

### Shared finite bounds for quadrics

VITRAL's `QuadricSurface::getBounds` now owns the positive-definite ellipsoid
bound adopted from POV, including mixed coefficients and translated centers.
The POV acceleration adapter consumes this same result; the second copy of the
bound calculation has been removed. The legacy `getMinMax` also uses the shared
bound. Complements, indefinite, singular, near-singular and degenerate cases
that this calculation cannot safely bound remain unbounded. Non-finite input or
arithmetic is reported as indeterminate, and POV's acceleration falls back to an
unbounded volume. This preserves the historical numerical thresholds; it does
not claim a rigorous floating-point outward-rounding guarantee.

Analytical tests cover a shifted ellipsoid with mixed terms, transformed
instances, the POV acceleration adapter, inverted and unbounded solids, tiny
coefficients and non-finite arithmetic. Solid interval tests also exercise the
mixed-term equation. Two additional offline fixtures show a rotated ellipsoid
and a pair of ellipsoids. General positive containment tolerance remains stage E
work; the zero-tolerance and ray interval operations are covered here.

Validation: all VITRAL examples built; 169 enabled VITRAL tests and 170 enabled
POV tests passed. The 40 prior example captures and ten checked ShadersExample
CPU/OpenGL images matched their references. Both new quadric fixtures rendered
visibly and identically in sequential and parallel modes. `renderAll.sh`
produced 108 baseline-identical images in 113.98 seconds (1309.44 user CPU
seconds), compared with 113.49 seconds (1295.04 user CPU seconds) in the
preceding iteration. Roth retained exactly the seven accepted RMSE discrepancies.
All 161 copied local base files match VITRAL.

### Shared box geometry and offline half-spaces

VITRAL `Box` now owns its two arbitrary axis-aligned corners, supporting the
POV box representation and the centered constructor already used by VITRAL.
Typed first-hit, crossings, solid intervals, containment, finite bounds, copying
and deferred surface detail operate on that same box. POV's `RayCastingBox`
owns the VITRAL box rather than another set of geometric coefficients, retaining
its historical hit thresholds, statistics and traversal policy. Complemented
POV boxes reverse typed crossing orientation, segment membership and boundary
normal orientation. The mutable corners remain available for the existing
heightfield water-level parser, and subsequent queries use the changed values.

`InfinitePlane` and `PlanarHalfSpace` now provide deferred detail with explicit
boundary identity and normal reversal; the POV plane adapter delegates to the
native half-space. The MIT reader accepts `box min-x min-y min-z max-x max-y
max-z` and `halfspace a b c d` (the solid is `ax+by+cz+d <= 0`), with invalid
inputs rejected. New offline scenes exercise offset boxes and a sloped
half-space. Tests query the actual parsed solids, including their ray intervals,
as well as independent copying, inversion and deferred normals.

The plan now defers only stage D work that actually depends on stage E's
world-space tolerance semantics to the new stage F. Zero-tolerance primitive,
placement and structural composition work remains in D and must be reviewed
before starting E; the former F–I stages are now G–J. Application traversal and
baked policies remain adapters until the scene/CSG and removal stages.

Validation: all VITRAL examples built; 172 enabled VITRAL tests and 173 enabled
POV tests passed. The 44 preceding example captures and all ten checked
ShadersExample CPU/OpenGL images matched their references. Both new fixtures
rendered visibly and identically in sequential and parallel modes. The full
`renderAll.sh` run produced 108 baseline-identical images in 115.39 seconds
(1317.65 user CPU seconds), compared with 113.98 seconds (1309.44 user CPU
seconds) in the preceding iteration. Roth retained exactly the seven accepted
RMSE discrepancies. All 161 copied local base files match VITRAL.

### Common structural CSG contract

VITRAL now defines the shared ordered `CsgExpression`/`CsgOperandView`
contract and the `CsgOperation` enumeration. Difference keeps operand zero as
the minuend and the remaining operands as subtractors. `CsgTree` provides a
borrowed-operand construction scaffold, checks visible cycles on insertion and
can itself be a nested operand; `CsgBodyOperand` exposes an existing body's
geometry and canonical affine placement without cloning it. Referenced operands
and bodies must outlive their containing expressions; mutation by an external
implementation remains subject to that implementation's lifetime and cycle
rules.

POV's existing CSG node and operands implement the same contract directly over
their existing storage and transform state. Its former operation type is an
alias of the common enumeration, so both existing evaluators retain their
behavior and operation identifiers without a parallel expression graph. Tests
exercise union, intersection, ordered difference, nested nodes, placement,
identity, live body updates, invalid indices and cycle rejection.

The stage D structural scaffold is complete at the requested scope. It is not
a ray evaluator or an offline-renderable geometry: the shared expression still
needs the G evaluators and scene/reader integration. The plan records the
union, intersection and difference sphere scenes as stage G deliverables,
rather than adding `.ray` files whose CSG content the reader would ignore.

Validation: all VITRAL examples built; 173 enabled VITRAL tests and 174 enabled
POV tests passed. The 48 example captures and ten checked ShadersExample
CPU/OpenGL shading images matched their references. `renderAll.sh` produced
108 baseline-identical images in 114.83 seconds (1306.11 user CPU seconds),
compared with 115.39 seconds (1317.65 user CPU seconds) in the preceding
iteration. Roth retained exactly the seven accepted RMSE discrepancies. All
161 copied local base files match VITRAL.

### Shared affine placement queries and zero-tolerance solid paths

VITRAL now exposes a validated geometry-to-parent affine pair through
`SimpleBody::queryAffinePlacement`. The common validation checks finite affine
matrices, the inverse in both composition orders and explicit singular or
indeterminate cases. The existing decomposed VITRAL controls retain their
established validation. POV supplies the same contract by composing and
converting its object and geometry placement layers, including shear, without
normalizing ray directions.

`SimpleBody::classifyRay` and zero-tolerance `classifyPoint` now consume this
shared pair. The POV native-geometry binding implements typed containment and
solid classification, including complement membership and boundary orientation,
so these queries work on a placed POV sphere as well as the other supported
primitives. Positive tolerance on POV affine placement remains explicitly
indeterminate for stage E/F. CSG operand views expose the same validated pair.
Tests cover a sheared, nonuniformly scaled two-layer POV instance, affine ray
parameters, complement, provenance, singular matrices and inconsistent pairs.

VITRAL's `SimpleBody::emitRayCrossings` now performs the same affine conversion
and appends instance provenance. It buffers results until the geometry query
succeeds, so a failed source query cannot leak partial events. Tests exercise a
reflected instance and the ordered crossing kinds and original ray parameter.

Stage D is still partial: this change unifies the query contract, not the
underlying authoritative placement storage. VITRAL's decomposed editing/render
state and POV's two matrix layers remain separately maintained, and POV's
legacy crossing emission has not yet been replaced by the common typed path.
Stage G still owns the CSG evaluators and the corresponding offline examples;
positive world-space tolerance remains in E/F.

Validation: all VITRAL examples built; 175 enabled VITRAL tests and 176 enabled
POV tests passed. The 48 example captures and ten checked ShadersExample
CPU/OpenGL shading images matched their references. `renderAll.sh` produced
108 baseline-identical images in 114.13 seconds (1308.44 user CPU seconds),
compared with 114.83 seconds (1306.11 user CPU seconds) in the preceding
iteration. Roth retained exactly the seven accepted RMSE discrepancies. All
161 copied local base files match VITRAL.

### Stage D closure: affine placement, crossings and basic bounds

`SimpleBody` now stores the validated forward/inverse `AffinePlacement` pair;
TRS edits update that pair and retain their existing optimized rendering caches.
A caller can set an arbitrary affine pair, including shear, and the regular
raytracer, bounds, zero-tolerance classification and crossings use that same
placement. Non-decomposable affine placements explicitly reject TRS rotation or
scale editing; translation and resetting to identity remain available. Tests
cover an explicitly sheared body, its ray parameter, point, transformed normal,
classification, translation and invalid-pair handling.

POV exposes the same canonical column-vector placement and accepts an affine
assignment while preserving its separate object and geometry layers. The
assignment updates the matrices actually used by tracing and disables TRS-step
coefficient folding for that body, avoiding a stale optimized representation.
Tests cover two layers, inversion, native crossing provenance and a baked quadric
after explicit shear. POV objects with separately owned bounding/clipping
regions currently reject arbitrary affine assignment; their ownership and editor
policies are explicitly deferred to F.

For unbounded/unclipped basic POV leaves, typed zero-tolerance crossings now go
through the common instance query, preserving the original affine parameter,
leaf identity, instance path and orientation. Projected triangles retain their
historical projected traversal as a policy; normal projected meshes expose their
finite shared bounds, while inverted projected faces are unbounded, including in
acceleration. Bodies with explicit bounding or clipping regions retain the
historical traversal until the scene-policy integration in F. VITRAL's existing
primitives provide ordinary shared bounds; the future geometry catalog is covered
in F/H as requested. The CSG structure remains the completed D scaffold, with
actual evaluators and offline CSG examples in G.

D is complete under the requested basic-primitive scope. Positive world-space
tolerance, region-specific typed traversal/affine editing and future geometry
families remain explicitly listed in F; they are not represented as completed.
Validation: all VITRAL examples built; 176 enabled VITRAL tests and 177 enabled
POV tests passed. The 48 existing example captures and ten checked ShadersExample
CPU/OpenGL images matched their references. `renderAll.sh` produced 108
baseline-identical images in 116.41 seconds (1317.92 user CPU seconds), compared
with 114.13 seconds (1308.44 user CPU seconds) in the preceding iteration. Roth
retained exactly the seven accepted RMSE discrepancies. All 161 copied local
base files match VITRAL.

### Stage E start: ellipsoid regression

Added world-space tolerance assertions for an ellipsoid with semi-axes 3, 2 and
1, including points on both sides of the tolerance band on two axes. The first
run failed all nine assertions because the current nonuniform placement returns
`Indeterminate`. Added a second regression for a sheared ellipsoid, using a
surface point with an analytically known world normal; it also fails for the
same reason. The assertions are retained and temporarily disabled in the normal
suite. Explicitly execute them with
`vitral_base_tests --gtest_also_run_disabled_tests --gtest_filter='GeometryQueryContract.DISABLED_*Ellipsoid*'`.
They must be enabled again once the implementation passes them.

The existing enabled VITRAL suite passed 175 tests. No production geometry or
rendering code changed in this step; the containment solver and the remaining
stage E numerical coverage are still pending.

### Stage E: conservative affine-sphere certificates

`SimpleBody` now uses the shared affine placement to classify certain positive
world-tolerance queries on nonuniformly scaled or sheared spheres. An enclosure
of the implicit quadratic certifies inside/outside, and an explicit transformed
surface witness certifies boundary proximity. Inconclusive queries return
`Indeterminate`; no estimate is treated as a complete nearest-distance solver.
The unchanged ellipsoid and shear regressions now pass and are enabled. Added
reflection, rotation, tolerance-threshold, singular and ill-conditioned cases.
The existing uniform-scale and zero-tolerance paths remain in place.

The arithmetic uses conservative bounds in exact arithmetic and a floating-point
guard, not a proven roundoff bound. A complete solver, a defensible numerical
error analysis, and independent numerical cross-checks remain open in E. The
certificates do not yet provide all results required by the CSG evaluators.

Validation for this increment: all configured VITRAL examples built; 178 enabled
VITRAL tests passed, followed by the updated focused ellipsoid tests. The POV
environment tests passed. All 48 captured examples and the ten CPU/OpenGL
shading-mode images matched their preceding references. `renderAll.sh` produced
108 baseline-identical images in 115.22 seconds (1314.33 user CPU seconds),
compared with 116.41 seconds (1317.92 user CPU seconds) in the preceding run.
The local VITRAL base mirror still matches all 161 copied files.
The Roth render completed all 108 scenes and its regression check retained
exactly the seven previously accepted RMSE discrepancies, with no new ones.

### Stage E: oblique ellipsoid witnesses

Added a regression derived from a known point and normal of an ellipsoid. The
previous implementation returned `Indeterminate` for points at signed normal
offsets of 0.095 with a tolerance of 0.1. It now searches the source sphere for
an explicit transformed surface witness; any successful witness is sufficient,
even when it is not the nearest surface point. The same regression also covers
an exterior point at offset 0.101: a supporting-plane lower bound now proves
that it is outside the tolerance band. Both assertions failed before their
respective changes and pass afterward.

The iterative search is bounded and cannot establish that a missing witness
does not exist. The supporting plane and existing quadratic enclosure are also
only sufficient certificates. Inconclusive inputs remain `Indeterminate`.
Formal floating-point bounds, independently computed numerical comparisons and
a complete distance query remain open in E.

An additional analytic normal test covers an oblique point under shear. All
180 enabled VITRAL tests passed, as did the POV environment tests. All 48
captured example images and ten CPU/OpenGL shading-mode images match the
preceding references. `renderAll.sh` reproduced all 108 reference images in
114.72 seconds (1297.38 user CPU seconds), compared with 115.22 seconds
(1314.33 user CPU seconds) in the previous run.
The Roth regression check again reports only the seven previously accepted
RMSE discrepancies, with no additional scenes.

### Stage E: interior quadratic bound and independent sampling

An ellipsoid interior point at world distance 0.2 from the surface, queried with
tolerance 0.19, initially returned `Indeterminate`. A quadratic upper bound
obtained by completing the square now certifies that case as inside. The bound
uses positive definite systems and remains a sufficient test: inconclusive or
numerically unsafe systems do not force a result.

Added an independent surface-grid comparison for five points on both sides of a
sheared ellipsoid. A geometric bound on grid spacing separates the chosen query
tolerances from the sampled distance. The test permits `Indeterminate` but
rejects definite results that contradict the grid bounds. This begins numerical
cross-checking; broader cases and rigorous floating-point error bounds remain
open, as does a complete nearest-distance solver.

Validation: all configured VITRAL examples built; 182 enabled VITRAL tests and
the POV environment tests passed. All 48 example captures and ten CPU/OpenGL
shading modes matched their previous references. `renderAll.sh` reproduced all
108 reference images in 113.58 seconds (1306.94 user CPU seconds), compared
with 114.72 seconds (1297.38 user CPU seconds) in the preceding run. The local
base mirror audit still reports 161 identical files.
The Roth regression check retained exactly the seven previously accepted RMSE
discrepancies, with no additional scenes.

### Stage E: centre witness and wider numerical checks

An ellipsoid queried at its centre previously returned `Indeterminate` even
when the tolerance reached an obvious surface point. The implementation now
tries source-axis surface witnesses when the radial seed is undefined; it
retains `Indeterminate` when a witness lies at the floating-point threshold.
The new centre regression was run and failed before this change, then passed.

Expanded the independent surface-grid comparison to combined translation,
rotation, reflection and shear at scale factors 0.01, 1 and 100, using a conservative
geometric grid-spacing bound. Added a test for a 1e-12 affine scale that keeps
an uncertain near-boundary query explicitly indeterminate. The grid test is a
numerical cross-check, not a proof of the floating-point guards or a complete
distance solver; these requirements remain open in E.

Validation: all configured VITRAL examples built; 185 enabled VITRAL tests and
the POV environment tests passed. All 48 example captures and ten CPU/OpenGL
shading modes matched their previous references. `renderAll.sh` reproduced all
108 reference images in 114.65 seconds (1308.83 user CPU seconds), compared
with 113.58 seconds (1306.94 user CPU seconds) in the preceding run. The local
base mirror audit reported 161 identical files. The Roth regression check
retained the seven known RMSE discrepancies and reported no additional scenes.

### Stage E: tighter exterior distance certificates

A normal-offset regression for a sheared ellipsoid initially returned
`Indeterminate` for a point at distance 0.1001 with a tolerance of 0.1. The
bounded surface-witness iteration now also tries the supporting plane at each
candidate surface point. A witness gives an upper distance bound, and any
supporting plane gives an exterior lower bound. The unchanged regression passes;
additional analytic normal offsets on both sides of the band reject incorrect
definite classifications. An exhausted or numerically uncertain search still
returns `Indeterminate`. This starts tighter distance bracketing, but does not
yet provide a complete nearest-distance query or proven floating-point bounds.

Validation: all configured VITRAL examples built; 187 enabled VITRAL tests and
the POV environment tests passed. All 48 example captures and ten CPU/OpenGL
shading modes matched their previous references. `renderAll.sh` reproduced all
108 reference images in 112.92 seconds (1302.36 user CPU seconds), compared
with 114.65 seconds (1308.83 user CPU seconds) in the preceding run. The local
base mirror audit still reports 161 identical files.
The Roth regression check retained exactly the seven previously accepted RMSE
discrepancies, with no additional scenes.

### Stage E: explicit affine ellipsoid distance

Added an explicit numerical distance query for affine sphere instances.
`OrthogonalEllipsoidDistance` solves the nearest-point secular equation,
including centre, repeated-axis and interior hard cases. For sheared instances,
`AffineEllipsoidDistance` computes principal axes from the symmetric
transformation covariance with a Jacobi iteration and checks its reconstruction
residual. Invalid, unsupported and numerically indeterminate cases return
explicit statuses without changing output arguments. `SimpleBody` exposes the
estimate for a placed sphere and reports other geometries as unsupported.

The query reports an estimated numerical guard, not a proven error interval;
containment and CSG do not consume it as a certified result. Tests compare
analytic normals and hard cases, a sampled surface, and twelve reference values
from independent multi-start constrained minimization, repeated with rotation,
translation and multiple scales. Singular and highly ill-conditioned transforms
are checked explicitly. Formal rounding bounds, more conditioning coverage and
safe integration with both CSG evaluators remain open.

The independent fixture generator is
`base/src/test/generate_ellipsoid_distance_reference.py` in VITRAL. It uses
multi-start constrained SLSQP and is not required by the C++ build or test suite.

Validation: all configured VITRAL examples built, 197 enabled VITRAL tests and
the POV environment tests passed, and all 48 example captures and ten CPU/GPU
shading-mode images matched their previous references. `renderAll.sh` produced
108 baseline-identical images in 114.47 seconds (1302.99 user CPU seconds),
compared with 112.92 seconds (1302.36 user CPU seconds) in the previous run.
The local base mirror audit reports 161 identical files.
The final Roth regression check retained exactly the seven accepted RMSE
discrepancies, with no additional scenes.

### Stage E: interval-verified scene-sphere containment

Added outward-rounded binary64 interval arithmetic and a certified distance
enclosure for affine spheres. The enclosure evaluates a surface witness, two
support-plane directions and interval cofactors of the stored forward matrix.
Interior lower bounds use the inverse norm, a quadratic spectral bound and a
residual-verified dual bound. The numerical method proposing a witness or dual
candidate is not trusted: every decision uses the interval result instead.

`SimpleBody::classifyPointCertified` returns inside, outside or boundary only
when the enclosure separates the answer from the tolerance, and otherwise
returns `Indeterminate`. Positive-tolerance scene-sphere containment now uses
that path instead of the earlier heuristic implementation, which was removed;
the zero-tolerance path is unchanged. Unsupported rounding environments,
overflow, uncertain singularity and invalid input retain explicit statuses.
The derivation and platform assumptions are recorded in
`doc/vitralCertifiedEllipsoidBounds.md`.

Tests cover the original ellipsoid expectations, shear and reflection, interior
and exterior near-threshold cases, independently generated constrained
minimization references, exact-rational binary64 interval cases, subnormals,
overflow and non-default rounding modes. Other primitives and CSG propagation
remain open in F/G. The nearest-distance estimator still returns an estimated,
not certified, error and is not used to force classification.

The independent comparison was extended to 24 additional points over three
distinct affine matrices; `base/src/test/generate_certified_bounds_reference.py`
in VITRAL regenerates the multi-start constrained-minimization fixtures. A
separate standard-library-only generator, `generate_interval_reference.py`,
recreates the exact rational arithmetic fixtures. The final suite passed 204
enabled VITRAL tests.

The placed-sphere query now decides all 84 near-band and separated cases in the
two independent affine fixture sets. At the ellipsoid centre, it tries and
refines multiple surface directions, but still accepts only interval-verified
decisions. A further exact-rational stress set covers 64 binary64 arithmetic
cases, regenerated by `generate_interval_stress_reference.py`.

Intrinsic `Sphere::classifyPoint` now uses outward intervals for positive
tolerance. A regression at the represented threshold was run before this change
and exposed an incorrect boundary result; it now returns `Indeterminate`.
Because the legacy integer adapter has no uncertainty value, it throws an
explicit `IllegalStateException` rather than mapping a failed or indeterminate
query to outside. Its zero-tolerance path is unchanged. POV's separate legacy
heuristic and composed CSG propagation remain F/G work.

Validation: all configured VITRAL examples built; 203 enabled VITRAL tests and
the POV environment tests passed. All 48 example captures and ten CPU/GPU
shading modes matched their references. `renderAll.sh` produced 108 identical
images in 112.60 seconds (1305.32 user CPU seconds), versus 114.47 seconds
(1302.99 user CPU seconds) in the preceding run. The local mirror audit reports
161 identical base files. The interval class was compiled with fast-math and
floating-point contraction disabled.
The final Roth regression check retained exactly the seven previously accepted
RMSE discrepancies and reported no additional scenes.

After the centre-search and intrinsic-sphere changes, the final VITRAL suite
passed 207 enabled tests and all configured examples built. The POV environment
tests passed; `renderAll.sh` produced 108 baseline-identical images in 113.85
seconds (1310.27 user CPU seconds), compared with 114.47 seconds (1302.99 user
CPU seconds) before this interval work. All 48 example captures and ten checked
CPU/OpenGL shading images matched their references. The base mirror audit
reported 161 identical copied files.
The final Roth regression check retained the seven accepted RMSE discrepancies
and reported no additional scenes.

### Stage E: certified minimum-distance enclosure

`AffineEllipsoidMinimumDistance` and `SimpleBody::encloseMinimumBoundaryDistance`
now return a world-space surface-distance enclosure with a caller-selected
maximum absolute width. The existing numerical solver provides only a candidate
witness. An interval-verified supporting bound and a Lagrange-dual certificate
bound the minimum independently of that estimate; positive definiteness is
verified using interval principal minors, and the residual is bounded using an
interval eigenvalue lower bound. `Valid` is returned only if the verified width
meets the request. Invalid, unsupported or inconclusive queries leave outputs
unchanged. The scene containment path uses the certified enclosure if its
initial witness search cannot decide.

Tests cover the interior hard case, centre, exterior, surface points, shear,
reflection, rotation, translation, non-unit radius, scales from 0.01 to 100,
wrong inverse hints, non-default rounding and 24 independent minimization
fixtures. The POV typed native sphere binding is also checked to preserve
`Indeterminate` through inversion. The original POV integer heuristic and CSG
composition remain integration work in F/G; the optional approximate solver's
own error value remains an estimate and is not used as a certificate.

POV's `RayCastingBody` now exposes the same estimated and certified queries
for native sphere leaves through the shared `SimpleBody` API. The adapter uses
the full layered affine placement, reverses certified inside/outside for an
inverted leaf, and explicitly rejects bodies with owned clipping or bounding
regions. Tests exercise virtual dispatch, layered transforms, inversion,
uncertain boundaries and unchanged outputs on unsupported regions. POV's
existing integer CSG traversal is unchanged; integrating the new typed outcomes
into that traversal remains F/G.

Adversarial validation also caught a nonzero transform column whose squared
length underflows. The orthogonal estimator now reports `Indeterminate` rather
than incorrectly declaring that transformation singular; outputs remain
unchanged. This failure remains explicit in the certified query as well.

The 24 independent affine fixtures now require a certified minimum enclosure of
width at most 1e-5, in addition to containment checks. Tests also cover
different scales, non-unit radius, a deliberately wrong inverse hint, an exact
surface point, an excessively narrow requested width and unsupported rounding.
The off-axis interior hard case is used by certified scene classification when
its initial witness search cannot decide.

Final validation: all configured VITRAL examples built, 212 enabled VITRAL
tests and the POV environment tests passed. All 48 example captures and ten
checked CPU/OpenGL shading images matched their previous references.
`renderAll.sh` produced 108 baseline-identical images in 113.69 seconds
(1318.55 user CPU seconds), compared with 113.85 seconds (1310.27 user CPU
seconds) in the previous run. The mirror audit reported 161 identical files.
The final Roth regression check retained exactly the seven accepted RMSE
discrepancies, with no additional scenes. Stage E meets its affine-sphere
distance/tolerance exit conditions; the scene-policy and CSG adoption cases
described above remain in F/G.

### Stage F: conservative point queries through shared CSG

`CsgOperandView` now exposes typed world-space containment, and `CsgBodyOperand`
forwards it through the body's virtual scene API. `CsgPointClassification`
traverses identity-placed shared expression trees and conservatively combines
union, intersection and ordered difference. It returns a definite inside or
outside only when operand results prove the entire tolerance ball has that
classification. Thus a definite result can mask an uncertain operand only when
the Boolean operation proves that operand irrelevant. Invalid inputs are never
masked. Operand boundary results that do not prove a composite result yield
`Indeterminate`, since another operand may hide the boundary; a one-operand
expression preserves its operand's result. Cyclic or empty expressions are
invalid, and separately placed nested expressions remain explicitly unsupported.

Tests cover nested compositions, translated sphere leaves, all three Boolean
operations, hidden boundaries, uncertainty, invalid input, and a mixed VITRAL/POV
composition through virtual dispatch. This is a point-classification contract;
it does not claim composite boundary attribution or implement the historical or
Roth event evaluators. Other primitives, clipping and bounding policies,
transformed child expressions, provenance, conservative culling and legacy
integer integration remain open in F/G.

Validation: all configured VITRAL examples built and 214 enabled base tests
passed (one pre-existing test is disabled); the POV environment integration
passed. All 48 offline and shader captures and the ten individual CPU/OpenGL
shading-mode captures matched their preceding references. `renderAll.sh`
produced all 108 baseline-identical images in 113.16 seconds, with 1301.30
user CPU seconds (preceding run: 113.69 and 1318.55 seconds). The mirror audit
found all 161 copied files identical to VITRAL.
The full Roth run rendered all 108 scenes; the regression check found exactly
the seven previously accepted RMSE discrepancies and no new ones.

### Stage F: affine CSG placement and exposed boundaries

A borrowed `CsgPlacedExpression` places a shared child expression without
changing or duplicating its geometry; failed placement edits preserve its prior
state. `CsgAffineTolerance` computes outward-rounded cofactor/determinant
intervals from the forward placement instead of trusting an approximate inverse.
It encloses the inverse image of the parent tolerance ball in a child-space
ball. A second, inscribed child ball is certified using a Frobenius-norm bound
and the interval error of the mapped center. The outer ball proves inside or
outside; the inner ball can prove that a child boundary lies within the original
tolerance. Overflow, determinant uncertainty and unsupported arithmetic remain
explicit. As in the stage E certificates, the guarantees require binary64
round-to-nearest, gradual underflow and correctly rounded square root; the
certificate code is compiled without fast math or contraction.

The CSG point policy now reports an exposed boundary when exactly one operand
has a certified boundary within the tolerance and all other operands are
certified non-interfering throughout that ball. It keeps multiple potentially
coincident boundaries indeterminate. `CsgPointWitness` optionally records the
borrowed root-to-leaf operand path and the parity of difference orientation when
a single operand proves the returned point result. A result requiring several
operands has no single-leaf witness. The witness is a point-query proof, not a
ray-hit record or a complete representation of all possible boundary owners.

Tests cover exposed, hidden and coincident boundaries; nested differences;
multiple parent placements; reflections and shear; inner/outer ball samples at
16 scale combinations; nearly singular placement; atomic failure; unsupported
rounding; and a placed mixed VITRAL/POV composition. The historical POV and Roth
CSG traversal remain unchanged until their evaluator integration in G. General
coincident-boundary attribution, typed composition crossings, tolerance-aware
bounds and remaining primitive and clipping policies remain open in F.

Final composition validation: all configured VITRAL examples built; 225 enabled
base tests passed, including propagation of invalid/unsupported inner probes,
with one pre-existing disabled test. POV environment integration passed. All 48
example captures and all ten individual CPU/OpenGL shading modes matched the
preceding references. The Roth run rendered all 108 scenes and retained exactly
the seven previously accepted RMSE discrepancies, without new scenes. The
base-mirror audit found all 161 copied files identical to VITRAL.
The final isolated `renderAll.sh` run produced all 108 baseline-identical images
in 114.63 seconds and 1317.74 user CPU seconds, compared with 113.16 seconds and
1301.30 user CPU seconds in the preceding stage F run; the difference is within
the observed run-to-run variation of the recorded baseline measurements.

### Stage F: multi-operand point proofs and typed ray events

`CsgPointProof` records sufficient leaf classifications, the query point and
world/parent-space tolerance used at each leaf, the borrowed operand paths and
difference orientation. Conjunctive outcomes now retain all necessary leaf
facts, including exposed-boundary decisions and nested placed trees. The paths
borrow their operand identities: proofs describe the scene at query time and
must be discarded or re-evaluated if that scene is subsequently edited. A
single-leaf witness remains available where appropriate. The simplification of
repeated operands is deliberately conservative: two views of the same body, or
exact toolkit spheres with identical radius and affine forward placement, denote
the same solid. Repeated union/intersection boundaries can then be attributed,
a solid subtracted from itself is empty under the point-query solid-set
semantics, and repeated subtractors do not create spurious multiple boundaries.
This does not prescribe how either renderer handles coincident surface events;
that remains an explicit G policy. Application-specific body subclasses
are not compared structurally. Merely nearby surfaces and general coincident
geometries remain indeterminate, as do inconsistent classifications of views
claiming identity.

`CsgRayLeafQuery` collects typed solid classifications for all leaf operands in
an ordered shared expression, with affine ray parameters, their individual
geometry provenance, root-to-leaf operand paths, parent-placement snapshots and
difference parity. Membership remains the raw leaf membership; normal orientation
is reversed for each enclosing subtraction. Invalid, unsupported or malformed
leaves clear the whole output. `CsgRayEventQuery` groups the reported events by
exactly equal binary64 parameter while retaining every leaf and boundary index;
it does not infer that numerically close values are geometrically coincident.
The batches are scaffolding for the distinct Boolean evaluator policies in G;
they do not claim to combine crossings into a composite solid.

A POV body with clipping or owned bounding regions now returns `Unsupported`
for typed solid-ray classification instead of silently classifying its raw
geometry. Unclipped/unbounded bodies delegate through the shared scene contract,
including binding inversion. Tests cover mixed POV/VITRAL coincident ray events,
box, half-space and quadric leaves, affine and nested-difference orientation,
malformed partial data, exact ties, nearby distinct roots and atomic failure.

Validation for this increment: all configured VITRAL examples built and 234
enabled base tests passed, with one pre-existing disabled test. POV environment
integration passed, including inverted and clipped/bounded leaves and coincident
mixed-project ray events. All 48 example captures and ten individual CPU/OpenGL
shading-mode images matched their references. The complete Roth run rendered 108
scenes and retained exactly the seven accepted RMSE discrepancies, with no new
ones. The base mirror audit found all 161 copied files identical to VITRAL.
The final isolated `renderAll.sh` run produced 108 baseline-identical images in
112.66 seconds and 1320.24 user CPU seconds, compared with 114.63 seconds and
1317.74 user CPU seconds in the preceding run.

### Stage F: exact family and placement equivalence

`CsgSolidEquivalence` recognizes conservative sufficient conditions for the
same placed solid across spheres, boxes, oriented planar half-spaces and quadrics.
It handles signed axis permutations and reflections, sphere radius/placement
rescaling, boxes with independently translated/scaled bounds (and certain general
affine edge reparameterizations), positively proportional plane and quadric
coefficients, and axis-aligned oriented planes with different placements. It also
recognizes spherical quadrics as spheres and linear quadrics as half-spaces where
the exact sufficient conditions hold. Distinct views of the same expression
and placement are comparable. The checks do not turn an unsupported containment
query into a definite classification.

`CsgExactArithmetic` compares binary64 products and affine endpoints as exact
dyadic integers; rounded endpoint equality is never used as a proof. It rejects
unsupported arithmetic environments. A deterministic corpus generated with
Python `Fraction` supplies 1,179 independent exact arithmetic checks, including
subnormal values, overflow-scale products and equal rounded but unequal exact
expressions. Tests additionally cover mixed quadric terms, permutations,
reflection, translation, scale, opposite orientations and nearby surfaces.

Scene bodies explicitly expose a borrowed comparable core solid only when their
application policies preserve it. The default permits the plain VITRAL body;
POV opts in its compatible sphere, box, quadric and half-space bindings; it
rejects wrapper-level inversion, clipping, owned bounds and unknown subclasses.
Inversion already encoded in quadric or plane coefficients remains part of the
comparison. Tests
compare both directions between POV and VITRAL and preserve coincident typed
ray events across a sphere/quadric pair. General rotations, arbitrary affine
reparameterizations, inversions and additional geometry families remain open;
absence of a proof does not mean the solids are different.

Family-equivalence validation: all configured VITRAL examples built and 244
enabled base tests passed, with one pre-existing disabled test. POV environment
integration passed, including comparable core solids from both applications,
inversion and scene-policy rejection, and cross-family ray events. All 48 example
captures and all ten CPU/OpenGL shading modes matched their references. Roth
rendered 108 scenes and retained exactly the seven accepted RMSE discrepancies,
with no new ones. The base mirror audit found all 161 copied files identical to
VITRAL.
The final isolated `renderAll.sh` run produced 108 baseline-identical images in
113.66 seconds and 1315.26 user CPU seconds, compared with 112.66 seconds and
1320.24 user CPU seconds in the preceding run.

### Stage F: affine equivalence and coincident boundary attribution

`CsgPointAttribution` supplies the complete set of owner paths for a boundary
when the existing point classifier can prove it: equivalent coincident operands
retain their individual identities and difference orientation, while hidden or
cancelled boundaries are excluded. If the boundary or owner set is ambiguous,
the result remains indeterminate or the attribution explicitly incomplete.
Paths borrow scene identities and must be re-evaluated after scene edits.

Exact sufficient equivalence now includes affine spheres whose scaled Gram
matrices agree, affine boxes with exactly matching edge vectors and a matching
corner even when local origins differ, and oriented planes or quadrics whose
exact world coefficients are positively proportional. The plane and quadric
conversion uses a homogeneous adjugate transform and verifies every rounded
intermediate against dyadic integer arithmetic; if any intermediate is not
exactly representable, it declines the proof. Spherical and linear quadrics can
also be compared across families in the supported cases. Equality of rounded
values alone is not accepted, and unsupported point-containment queries are not
silently resolved merely because another operand is geometrically equivalent.

The independent rational corpus was extended to 1,379 product, affine and
sum-of-products fixtures. Focused tests cover general non-permutation ellipsoid
placement, translated/sheared boxes, reflected oblique half-spaces, general
affine mixed quadrics, nearby unequal shapes, multiple coincident owners,
subtraction orientation, hidden cancellation and mixed POV/VITRAL attribution.
The final attribution validation also includes 50 independently generated
homogeneous quadric transformations computed with Python rational arithmetic,
atomic failure on an unrepresentable intermediate, and attribution through a
placed nested difference. The CSG arithmetic corpus contains 1,379 independent
product, affine and sum-of-products fixtures.

Affine-equivalence/attribution validation: all configured VITRAL examples built
and 253 enabled base tests passed; one pre-existing test remains disabled. POV
integration passed, including coincident owner paths across VITRAL and POV.
All 48 example captures and all ten CPU/OpenGL shading modes matched their
references. Roth rendered all 108 scenes and retained exactly the seven accepted
RMSE discrepancies, with no new ones. The mirror audit found all 161 copied base
files identical to VITRAL.
The final isolated `renderAll.sh` run produced all 108 baseline-identical images
in 111.91 seconds and 1320.58 user CPU seconds, compared with 113.66 seconds and
1315.26 user CPU seconds in the preceding run.

### Stage F: certified results across equivalent operands

Point classification evaluates all sibling operands first. An unsupported or
indeterminate **leaf** may then reuse a definite certified result from a sibling
only when both scene views prove they denote the same placed solid. Invalid
inputs are never replaced; inconsistent definite results remain indeterminate.
The proof records both the original certified leaf fact and an explicit
source-to-recipient equivalence relation, including both operand paths and their
difference orientation. Boundary attribution retains the recipient as a separate
owner. Tests exercise both operand orders, nested placement/difference, unrelated
nearby and complementary solids, unresolved uncertainty, and a mixed POV
quadric/VITRAL sphere. A clipped POV body cannot use this shortcut.

Separately built shared CSG trees and placed-expression views can now prove
structural equivalence when placements match exactly and their children are
equivalent. Arbitrary application views or expression implementations are not
unpacked for structural equivalence, since they may carry additional policies.
Point and typed-ray traversal also require both the operand view and child
expression to opt in explicitly that their structure fully describes the solid;
otherwise they return Unsupported. Subclasses of the shared tree and placement
views must opt in themselves rather than inheriting that promise automatically.
Union and intersection
operands may be reordered; difference retains its first operand and permits the
subsequent subtractors to be reordered. Invalid, cyclic, structurally different or
unproven graphs remain non-equivalent. This is a solid-set comparison; it does
not select materials or prescribe either renderer's coincident-event policy.

Validation: all configured VITRAL examples built and 258 enabled base tests
passed, with one pre-existing disabled test. POV integration passed. All 48
example captures and ten CPU/OpenGL shading modes matched their references.
Roth rendered all 108 scenes and retained exactly the seven accepted RMSE
discrepancies, with no new scenes. The base mirror audit found all 161 copied
files identical to VITRAL.

The structural traversal guard was additionally tested with opaque scene views,
opaque child expressions, subclasses with additional policies and cyclic graphs.
These cannot be silently treated as ordinary shared trees. Equivalence itself
unpacks only the known shared tree and placed-expression implementations.

Final policy-guard validation: all configured VITRAL examples built and 261
enabled base tests passed, with one pre-existing disabled test. POV integration
passed. All 48 example captures and ten CPU/OpenGL shading modes matched their
references. Roth rendered all 108 scenes and retained the seven accepted RMSE
discrepancies, with no new scenes. The base mirror audit found all 161 copied
files identical to VITRAL.
The final isolated `renderAll.sh` run produced all 108 baseline-identical images
in 114.51 seconds and 1319.92 user CPU seconds, compared with 111.91 seconds and
1320.58 user CPU seconds in the preceding recorded run.

### Stage F: associative compositions and certified subtree reuse

Shared union and intersection expressions now recognize associative and
idempotent regrouping as solid-set equivalence. Difference keeps the minuend
ordered while allowing equivalent unions of subtractors to be regrouped and
deduplicated. Flattening is restricted to known shared structural nodes with
identity placement; displaced subtrees, unknown policies and cycles are not
silently flattened. This comparison does not impose any material or event policy
on the renderers.

A certified sibling proof can now support an equivalent, structurally safe
subtree whose own point query is unsupported or indeterminate. The proof retains
both source and recipient paths and the identity relation. For a composite
recipient the classifier deliberately leaves the complete owner attribution
unavailable unless its own owner paths have been independently resolved; it
never fabricates a leaf owner. Tests cover regrouped/duplicated Boolean trees,
ordered differences, displaced non-equivalences, proof transfer and incomplete
attribution for an equivalent sphere/quadric subtree pair.

Validation: all configured VITRAL examples built and 263 enabled base tests
passed, with one pre-existing disabled test. POV integration passed, including
proof reuse across equivalent POV/VITRAL subtrees and incomplete attribution.
All 48 example captures and ten CPU/OpenGL shading modes matched their
references. Roth rendered all 108 scenes and retained the seven accepted RMSE
discrepancies, with no new scenes. The base mirror audit found all 161 copied
files identical to VITRAL.
The final isolated `renderAll.sh` run produced all 108 baseline-identical images
in 112.01 seconds and 1314.97 user CPU seconds, compared with 114.51 seconds and
1319.92 user CPU seconds in the preceding recorded run.

### Stage F audit

An audit of both projects before closing F recorded the non-conformities F-NC1
to F-NC6 and the risks F-R1 to F-R3 in the plan. CSG point leaves supported
only spheres; box and half-space positive tolerance and the uniform-scale body
path used round-to-nearest arithmetic; POV and VITRAL used two different
placed-sphere searches; several integer adapters converted uncertainty into
`OUTSIDE`; placed bounds could shrink by an ulp and CSG expressions had no
bounds. Stale stage letters in this record were corrected. Further exact
equivalence work was frozen, and proof versioning and workspace reuse were
assigned to G.

### Stage F: certified containment for basic families (F-1, F-6)

`Geometry` gained `classifyPointCertified` for a local closed ball and
`classifyBoxCertified` for a local box; both default to `Unsupported`.
`Sphere`, `Box`, `PlanarHalfSpace` and `QuadricSurface` implement them with
outward-rounded `FloatingInterval` arithmetic; their positive-tolerance
`classifyPoint` paths delegate to the certified routine and their
zero-tolerance paths are unchanged. `FloatingInterval` keeps sums and products
of point intervals exact when an error-free transformation proves it.
`AffineToleranceMap` (formerly `CsgAffineTolerance`) also reports per-axis
local half-widths, and its inner ball uses the smaller of the Frobenius and
`sqrt(||A||_1 ||A||_inf)` bounds.

`SimpleBody::classifyPlacedPointCertified` is the single placed-solid routine:
identity placement, the enclosing local box, outer and inner balls, and a
two-point world proof over fixed directions. The uniform-scale shortcut was
removed. `RayCastingBody` classifies its core solid through the same routine,
so a placed POV sphere and a placed VITRAL sphere receive the same certificate.
`CertifiedContainmentContract` checks all four families under nonuniform scale,
shear and reflection, including a soundness sampler that compares every
definite result with dense exact sampling; deliberately weakening the box
enclosure made that sampler report 44 failures.

### Stage F: legacy adapters and estimator (F-2, F-7)

Integer predicates inherited from the Java port keep Java parity and are
documented as legacy predicates; the Java sources contain the same defaults,
including the `-INSIDE` surface value of `InfinitePlane`. Adapters introduced
during convergence (`Sphere`, `PlanarHalfSpace`, `QuadricSurface`,
`NativeGeometryBinding`) throw `IllegalStateException`. No typed scene or CSG
path calls `doContainmentTest`. `estimateBoundaryDistance` and
`AffineEllipsoidDistance` are used only to propose candidates inside
`AffineEllipsoidMinimumDistance`, whose result is independently enclosed.

### Stage F: conservative bounds and culling (F-3)

`GeometryBounds::transformed` rounds outward, and `inflated`, `intersecting` and
`excludesBall` were added. `CsgOperandView::getSolidBounds` and `CsgSolidBounds`
give conservative bounds for shared expressions: unions enclose all operands,
intersections intersect them and differences use the minuend; placed children
are transformed and cycles are rejected. `CsgPointClassification` culls a leaf
as `Outside` only when its solid bounds exclude the tolerance ball.
`ConservativeBoundsContract` covers rounding, inflation, expression bounds and
culling.

### Stage F: POV region policies and placement editing (F-4, F-5)

`RayCastingBody` separates its core solid from its regions. A `clipped_by`
point query evaluates a shared CSG intersection of `RayCastingCoreOperand` and
one operand per clip, so attribution and uncertainty follow the common
classifier; a missing clip is `Unsupported`. A `bounded_by` region is used only
after `classifyWorldBoxCertified` proves that it contains the core bounds;
otherwise the query is `Unsupported`, so a bound never changes membership.
Solid world bounds of clipped bodies intersect the clip bounds. Ray
classification with regions remains `Unsupported` for G.

`setAffinePlacement` moves owned regions by `new * old^-1` after checking that
every region can move, so an edit either moves all regions or changes nothing.
Integration tests cover clipped, hollow and bounded spheres, tight and
non-containing bounds, region movement, return to identity and atomic
rejection.

### Stage F closure

All F exit-checklist items hold. VITRAL built all configured examples and passed
279 base tests, with one pre-existing disabled test; POV passed 280 tests.
All 48 example captures matched their references. `renderAll.sh` produced all
108 baseline-identical images in 115.88 seconds and 1307.26 user CPU seconds.
Roth rendered all 108 scenes and retained the seven accepted RMSE
discrepancies, with no new scenes. The audit mapped 81 raycasting files and
found all 161 copied base files identical to VITRAL.

### Roth gate re-rendering

`checkVitralConvergenceRoth.py` compares the images already present in
`outputCsgByRaySegments`; it does not render them. The images used by the
stage F gates dated from 2026-11-09. A fresh `renderCsgByRaySegments.sh` run on
the closed F code rendered all 108 scenes in 206 seconds and kept exactly the
seven accepted discrepancies, so the F result holds. The gate now re-renders
the Roth outputs before comparing them. With `-parallel`, 28 scenes that use
`rand()` textures differ between runs; the remaining 80 are byte-deterministic
and serve as the byte-identity check for Roth changes.

### Stage G: shared CSG evaluators

`CsgRothRayClassifier` classifies a shared expression along a ray from the leaf
batch of `CsgRayLeafQuery` and the exact-parameter groups of
`CsgRayEventQuery`, applying each group before evaluating Table 3 over the
expression. `CsgRayClassification` keeps the leaf snapshots, the combined
`RaySolidClassification` and one owner group per boundary.
`CsgCrossingMembershipFilter` selects leaf events with the POV membership
rules and certified sibling classification; `CsgRaySurfaceEvents` keeps them
ordered by parameter and leaf. `CsgMembershipRule` holds the Table 3 fold and
the heuristic's containment requirement.

`CsgRothRayClassifierContract` covers the three operators with owners,
touching and coincident boxes, interior starts, a half-space operand, failure
propagation and placed nested differences. A random-tree test compares every
segment with certified point classification at its midpoint on 200 trees of
spheres and boxes; replacing the difference rule with a union made it report
125 failures. `CsgCrossingMembershipFilterContract` shows the heuristic's
internal union surfaces and duplicate coincident faces, and that an uncertain
sibling fails the query unless another sibling proves exclusion.

The POV `ConstructiveSolidGeometryByRaySegment` merge and point containment use
`CsgMembershipRule`. `RayCastingBody::classifyRay` classifies verified
`bounded_by` bodies by their core and `clipped_by` bodies with the Roth
classifier over `RayCastingCoreOperand` and the clips; integration tests cover
clipped, hollow, verified-bound and unverified-bound spheres. After the change
the 80 deterministic `-csgRoth` renders were byte-identical to the fresh
baseline and the Roth check kept the seven accepted discrepancies.

Validation: VITRAL built all configured examples and passed 286 base tests, with
one pre-existing disabled test; the new evaluator tests are clean under
AddressSanitizer and UndefinedBehaviorSanitizer. POV passed 287 tests. All 48
example captures matched their references. `renderAll.sh` produced all 108
baseline-identical images in 115.07 seconds and 1316.33 user CPU seconds,
measured without concurrent load. The re-rendered Roth outputs kept the seven
accepted discrepancies, and the audit found all 161 copied base files identical
to VITRAL.

### Stage G: result versions and workspaces (G-5)

`CsgVersionStamp` captures the structure version of every expression node and
the modification version of every operand reachable from a root, and reports a
result current only when every node and operand tracks its edits and none has
changed. `CsgTree`, `CsgPlacedExpression`, `CsgBodyOperand` and
`RayCastingCoreOperand` report versions. `CsgRayClassification`,
`CsgRaySurfaceEvents`, `CsgPointProof` and `CsgPointAttribution` capture a
stamp when they are built. `CsgVersionStampContract` checks that a body edit, a
placement edit and an added operand each make Roth results, heuristic events
and point proofs stale, and that an untracked operand gives an incomplete
stamp.

`CsgRayWorkspace` holds the traversal path, event cursors, the current event
group and the per-leaf states. `CsgRayLeafQuery` fills a pooled
`CsgRayLeafBatch` in place, `CsgRayEventQuery` streams exact-parameter groups,
and `CsgRayClassification` stores owners as one flat list. Assignments of
`RayInstancePath`, `CsgPointWitness`, `RaySolidClassification` and
`CsgRayLeaf` reuse their storage. Measured with valgrind on a three-leaf
difference of a union: the original classifier made about 297 allocations per
query; with a reused result and workspace a run of 1, 101 or 1,001 queries makes
the same 126 allocations in total, so a query allocates nothing after warm-up.
A query takes about 5.5 microseconds with or without the workspace.
`CsgRayWorkspaceContract` checks that reused storage gives the same results as
fresh queries across alternating expressions and that a failure leaves the
result empty. The typed heuristic still makes about 3,600 allocations per query
on the same tree, almost all inside certified point classification.

Sanitizer builds of VITRAL must not use the full `cpp` tree:
`SceneEditorApplication` redirects `vitral_base` into its source-tree output
directory, which the regular test binaries load, so a sanitizer build replaces
that library. The library was rebuilt in Release after the sanitizer run.

### Stage G: POV evaluators against the shared evaluators (G-7)

POV `CsgOperand` now implements typed ray and certified point classification
for its leaf, through the shared solid of its POV wrapper and the shared
placed-solid certificate, which `SimpleBody` exposes publicly for this purpose.
`CsgOperand` reports a modification version that its transforms and inversion
advance, and `ConstructiveSolidGeometry` reports a structure version and opts
into structural traversal for its Morgan and Roth subclasses. The shared
evaluators therefore run on the expressions the POV parser builds.

`vitral_raycasting_tests` compares them on four fixtures along one ray. For
two overlapping spheres, the Roth render's crossings equal the
`CsgRothRayClassifier` boundaries and the Morgan render's four crossings equal
the `CsgCrossingMembershipFilter` events. For a Morgan difference built as the
parser does, an intersection with an inverted subtrahend, the render and both
shared evaluators give the same four parameters. Two touching boxes show the
POV Roth tie policy, which reports the shared face twice, against the
regularized single segment of the grouped policy. Two coincident spheres show
the boundary-inclusive heuristic keeping both faces, as the Morgan render does,
against one Roth boundary with two joint owners. Changing one expected value
made the test fail at that check.

### Stage G: offline CSG examples (G-6)

`CsgBody` is a VITRAL `SimpleBody` whose solid is a shared expression over the
leaf bodies it owns: node 0 is the root and nested nodes are identity-placed
`CsgTree` operands. It evaluates rays with `CsgRothRayClassifier` or
`CsgCrossingMembershipFilter`, as selected by `CsgBodyEvaluator`, after a slab
test against the conservative `CsgSolidBounds`. The hit is the first boundary
or selected event beyond `VSDK::EPSILON`. Detail comes from the leaf body named
by the boundary provenance; because `SimpleBody::doExtraInformation` reads the
ray stored in the hit, the local ray is stored there before the call. The normal
is reversed for reversed boundaries and mapped with the body's placement.
`ReaderMitScene` reads `csg union|intersection|difference [roth|pov]` … `end`
blocks, which nest and take `sphere`, `box`, `halfspace` and `quadric` operands
with the current surface. It rejects unknown operations, unterminated blocks,
an evaluator on a nested block and triangles inside a block.

Four scenes were added to `etc/geometry/mitscenes`. `csg_sculpture.ray` is a
box intersected with a sphere, minus a union of three axis cylinders;
`csg_sculpture_pov.ray` is the same solid with the heuristic, at 320×240.
`csg_lens.ray` holds an intersection of two spheres and a sphere with a
spherical bite, and `csg_union_pov.ray` holds two overlapping spheres with the
heuristic. The surfaces, holes, bite and shadows are as expected. With the
other evaluator `csg_lens.ray` and `csg_union_pov.ray` render pixel-identically.
The heuristic sculpture differs in 15,933 pixels at 640×480: the nested union
keeps the internal cylinder surfaces, which are visible inside the holes and
block the light through the vertical hole. `SimpleRaytracer` has no refraction,
so a transmissive union looks the same with either evaluator.

`runAll.sh` and `runAll.sh --parallel` rendered all 25 scenes. The sequential
and parallel images are byte-identical, and the 21 existing scenes are
byte-identical to their previous renders. Single-scene times at 640×480,
sequential and parallel: `box_offsets` 0.72 and 0.05 seconds, `csg_lens` 14.51
and 0.65, `csg_sculpture` 13.91 and 0.91, `csg_union_pov` 8.81 and 0.51; the
heuristic sculpture took 189.88 and 5.67 seconds before it was reduced to
320×240.

`CsgBodyContract` checks a difference's entry and hole hits, with distances,
outward normals, leaf materials, body placement, world bounds, certified point
classification and Roth ray classification. It also checks that, from inside
a hole made by a nested union, Roth reports the far wall and the heuristic the
internal union surface. `ReaderMitCsg` checks both sculpture scenes (structure,
evaluator, materials, the axis ray of the vertical hole, which only the
heuristic hits, and a top-face hit) and the rejected malformed blocks. Removing
the reversed-boundary normal flip made both `CsgBodyContract` tests fail.

### Stage G closure

All G exit-checklist items hold. VITRAL built all configured examples and passed
292 base tests, with one pre-existing disabled test; POV, rebuilt against the
same VITRAL base, passed 293 tests. The example capture produced 56 images,
eight more than before from the four CSG scenes, and all 48 with references
matched them. `renderAll.sh` produced all 108 baseline-identical images in 116
seconds and 1311.58 user CPU seconds. The re-rendered Roth outputs kept the
seven accepted discrepancies, with no new scenes, and the audit mapped 81
raycasting files and found all 161 copied base files identical to VITRAL.

The 56 images of that capture are the capture references for later gates. The
new CSG images are byte-identical sequentially and in parallel, with SHA-256
`e6f9ae64d97a4a002cc7706e5a9fb85f8d1bcf7ba9a1b15e603fe0a2abce8313`
(`csg_lens`), `dfcec95ac6a3bb50bc9e258bbe821db6018105186681494496962feb234a8e36`
(`csg_sculpture`), `82308884072e1ad5044049fa6eee319695a55bda1775af5961b8cda4ae2b26af`
(`csg_sculpture_pov`) and
`b6795e5dd101588ab37f9c80672280011eb88d73d480fc5fffe5a6aab15ae450`
(`csg_union_pov`). A standalone AddressSanitizer and UndefinedBehaviorSanitizer
build of `cpp/base` with isolated output ran the 15 CSG evaluator, version,
workspace, `CsgBody` and MIT reader tests without errors or leaks, and left the
shared `vitral_base` library unchanged.
