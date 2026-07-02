# Scene-Owned Baked CSG Performance Plan

## Scope

This plan focuses on the remaining performance gap between the older baked
branch and the current scene-owned baked representation for CSG-heavy scenes,
with `level3/drums2/drums.pov` as the primary target.

Reference commits:

- Older baked baseline:
  `4af1a75e5b7356600bec34e12a4882560994a058`
- Scene-owned baked branch under comparison:
  `455582a941b1c7076c32ff1df95d04ee2b55b0eb`

Primary benchmark:

- Scene: `level3/drums2/drums.pov`
- Resolution: `320x200`
- Output format: TGA
- Timing mode: optimized build for wall-clock measurements, `-pg` build for
  `gprof` attribution

The goal is not to reintroduce the old mutable geometry-baking model. The goal
is to keep the scene-owned baked architecture and recover the directness of the
older baked hot path where profiling proves that the current layer adds
structural overhead.

## Architectural Guidance

The purpose of this work is to optimize the refactored renderer while keeping
the `geometry` layer clean.

The older baked branch is useful as a performance reference, but it is not the
architecture to restore. In particular, optimization work must not reintroduce
scene/object transforms, baked ownership state, or render-specific traversal
policy into `Geometry` implementations.

The intended ownership split is:

- `environment/geometry`: primitive math, containment tests, normals, and
  geometry-local intersection routines
- `environment/scene`: parsed scene ownership plus immutable tracing
  compilation, including composed transforms, baked CSG records, stable object
  flags, bounds, and traversal metadata
- `render`: consumes the scene-owned baked representation and performs tracing
  without mutating geometry objects

This constraint is the central challenge of the plan: recover most of the
older baked branch's direct hot-path performance without moving transform
baking back down into `Geometry`. Any proposed optimization that depends on
mutable geometry-level transforms should be rejected or redesigned as a
scene-owned baked specialization.

## Current Diagnosis

The `drums` profile shows that the current branch is slower even though the
primitive scene content is similar and the current branch performs fewer
quadric primitive tests.

Optimized `320x200` timing:

| Commit | Meaning | Runs (s) | Ratio |
| --- | --- | ---: | ---: |
| `4af1a75` | older baked baseline | `5.11`, `5.12`, `5.12` | 1.00 |
| `455582a` | scene-owned baked layer | `11.29`, `11.26`, `11.28` | 2.20 |

Instrumented `-pg` timing:

| Commit | Time (s) | Notes |
| --- | ---: | --- |
| `4af1a75` | 12.01 | dominated by primitive intersections and Morgan CSG |
| `455582a` | 30.80 | dominated by baked CSG scaffolding |

Render statistics:

| Metric | Older baked baseline | Scene-owned baked branch |
| --- | ---: | ---: |
| Rays | 141879 | 141821 |
| Sphere tests | 947382 | 1294661 |
| Plane tests | 91153576 | 91245455 |
| Quadric tests | 68775936 | 41178805 |
| Shadow ray tests | 6159971 | 6506812 |
| Reflected rays | 46899 | 46841 |
| Transmitted rays | 30980 | 30980 |

Key reading:

- The current branch is not slower because it performs more primitive math.
- The current branch is slower because time moved into the scene-owned baked
  CSG scaffolding: operand dispatch, segment construction, containment checks,
  scratch queue traffic, and local ray cloning.
- Similar source geometry does not imply similar runtime cost once the hot path
  is routed through a more generic scene-baked representation.
- The current baked layer should be treated as an incomplete compiled
  representation until proven otherwise. It already builds
  `CompiledTracingScene` records before render, but those records still carry
  many pointers back to parsed scene objects and still route hot traversal
  through generic geometry/operand APIs. The first optimization step is
  therefore not another CSG micro-specialization; it is to make the final
  render consume a deliberately finalized baked scene and to measure how much
  original-scene fallback remains.

Older baked baseline `gprof` hotspots:

| Function | Self % | Calls |
| --- | ---: | ---: |
| `InfinitePlane::doIntersectionForAllRayCrossings` | 20.87 | 91.1M |
| `ConstructiveSolidGeometryByMorganRules::allCsgIntersectIntersections` | 20.05 | 55.1M |
| `Quadric::doIntersectionForAllRayCrossings` | 18.16 | 68.8M |
| `Quadric::doContainmentTest` | 8.67 | 69.1M |
| `PriorityQueuePool::push` | 8.13 | 101.4M |

Scene-owned baked branch `gprof` hotspots:

| Function | Self % | Calls |
| --- | ---: | ---: |
| `buildRaySegments` | 17.80 | 44.9M |
| `traceOperandAllCrossings` | 16.79 | 150.7M |
| `traceAllCrossingsWithScratch` | 6.19 | 80.3M |
| `containmentTestOperand` | 5.05 | 108.3M |
| `InfinitePlane::intersectPlane` | 3.91 | 90.2M |
| `RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)` | 3.91 | 85.5M |
| `PriorityQueuePool::pop` | 3.79 | 93.8M |

The current image difference against the older baked baseline is small and
localized:

- `AE=71` at `320x200`
- transmitted rays are unchanged
- reflected rays differ by only 58
- the delta is concentrated around CSG / reflective boundary pixels

This difference should be monitored, but it should not dominate this
performance plan unless a future change enlarges it or turns it into a broad,
structured heatmap.

## Measurement Gate

Every implementation phase must pass the full correctness gate:

```bash
./scripts/clean.sh
./scripts/compile.sh
./scripts/renderAll.sh
./scripts/testAgainstGoldenImages.sh
```

The image gate is acceptable when every reported scene is below `100000` in
the test metric, except for explicitly documented scenes such as
`level3/ionic5` where the heat map remains limited to blue-level pixels. Any
scene over the threshold with yellow/red heat-map structure is a regression
unless the phase documents and justifies a known pre-existing exception.

Performance phases must also be measured before and after with:

- optimized `drums` at `320x200`, at least three runs
- `drums` under `gprof` when a phase claims to reduce a known hot function
- image comparison against the immediately previous accepted branch
- image comparison against the older baked baseline to ensure the known
  `AE=71` pattern does not grow unexpectedly

A phase is acceptable only if it either:

- reduces `drums` wall-clock time measurably without worsening image stability,
  or
- clearly reduces the targeted hot counter and prepares a later phase, without
  regressing wall-clock time.

For the baked-scene-finalization phases, the main acceptance criterion is
structural: after parse/postprocess and before the first render ray, the
renderer must consume the finalized baked representation, and any fallback to
parsed scene objects must be explicit, measured, and temporary.

## Known Dead Ends

Do not repeat these approaches as generic optimizations:

- Generic early first-hit shortcuts that bypass `doIntersectionForAllRayCrossings`
  broadly. Earlier attempts preserved some spot checks but regressed the timing
  panel badly because they changed where expensive traversal work was paid.
- Top-level BVH or camera-distance ordering over bounded objects. The simple
  linear broad phase was faster for the measured panel; more traversal
  structure added overhead and ordering risk.
- Late shadow range filtering with `minT` / `maxT` after full candidate
  generation. It did not reduce primitive work and made the CSG shadow path
  slower.
- Containment-side baked `AABB` rejection for CSG operands. It is fragile;
  previous versions produced image deltas on scenes such as `pacman`.
- Reusable local-ray scratch stacks inside `BakedCsgTracing`. The first pass was
  exact for `drums` but regressed runtime to about `12.1s`, likely because the
  heap-backed scratch and extra reset logic cost more than stack construction.
- Sharing CSG scratch ownership from `BakedSimpleBodyTracing`. The first pass
  was image-stable but regressed `drums` to about `11.75s`; the wider lifetime
  and extra plumbing increased hot-path overhead.
- Generic transformed simple-operand native emission that still uses a local
  scratch queue. It was exact but timing-neutral around `11.18s` to `11.28s`,
  so it did not remove the real cost.
- Switching broad intersections from interval construction to Morgan-style
  containment as a blanket rule. Earlier experiments avoided some segment work
  but paid too much in containment filtering and did not improve `drums`.
- Primitive-type `dynamic_cast` direct-write logic inside `BakedCsgTracing`.
  It changed traversal behavior enough to increase primitive tests and badly
  regress `drums`.
- Direct plane-containment bypass inside `SingleCorePlaneIntersection`.
  The variant avoided the `containmentTestOperand` switch for plane clippers
  and preserved `AE=0`, but measured `11.01`, `10.96`, `11.00` seconds on
  `drums`, worse than the accepted baked-plane range. Do not retry this as a
  micro-optimization without a new profile showing containment dispatch has
  become dominant.
- Routing CSG first-hit through `traceSingleCorePlaneIntersection`.
  The variant was exact (`AE=0`) but regressed `drums` to `11.02`, `11.28`,
  `11.21` seconds. The specialized all-crossings path generates more queue
  traffic than the existing first-hit membership path for this case.
- Plane-only shortcut inside `traceFirstHitByIntersectionMembership`.
  The variant bypassed the local queue for plane operands only, but regressed
  `drums` to `11.84`, `11.85` seconds. The extra candidate construction and
  validation outweighed the queue avoidance.

These failures are useful constraints: the next work must be narrower,
pattern-aware, and validated against the actual `gprof` hotspots rather than
adding more general-purpose scaffolding.

## Phase 1: Establish The Final Baked Scene Boundary

Target:
Make the renderer's pre-render state a clearly finalized baked scene, not a
partially compiled side cache over the parsed scene tree.

Why:
The code already calls `Scene::buildCompiledTracingScene()` through
`Scene::setObjects()`, and the render loop already consumes compiled object
lists. However, this happens during parsing, before the final parse/postprocess
boundary, and the baked records still store many pointers back to
`SimpleBody`, `CsgOperand`, `ConstructiveSolidGeometry`, and `Geometry`.
That means the current baked layer is useful, but it is not yet a strong
preprocessed render representation. The performance gap is consistent with a
hot path that still reinterprets scene structure per ray.

Required behavior:

- add or enforce one explicit finalization point after parse and scene
  postprocessing and before `RenderEngine::initializeRenderer()`
- rebuild/freeze the compiled tracing scene at that point, so later parse-time
  mutations cannot leave stale baked data
- make the renderer read top-level traversal, shadow traversal, composites,
  simple bodies, and CSG through finalized baked arrays
- keep parsed scene ownership alive only as backing storage while the migration
  is incomplete
- document every remaining render-time fallback to parsed scene APIs, including
  `entry.object` fallback paths, `SimpleBody` transform/detail callbacks, and
  `CsgOperand` ownership/detail callbacks
- add temporary counters or diagnostics that can prove whether those fallbacks
  are used during `drums` and the full render gate

Exit criterion:

- the full correctness gate passes under the `100000` metric rule and the
  blue-only heat-map exception rule
- `drums` timing does not regress materially
- the code has a single obvious pre-render baked-scene finalization point
- fallback counters show whether render still reaches parsed scene objects,
  and any remaining fallback is intentionally listed for later removal

## Phase 2: Make Baked Records Execution-Oriented

Target:
Convert the final baked scene from a thin pointer/index cache into an execution
representation that hoists stable decisions out of the per-ray hot path.

Why:
`BakedSimpleBody` and `BakedCsgOperand` currently cache useful flags, matrices,
bounds, and nested CSG indices, but they still dispatch by repeatedly checking
transform state, nested CSG state, primitive type, annotation capability,
material ownership, and containment policy. A baked scene should preclassify
those decisions once.

Required behavior:

- add compact execution variants for baked objects and CSG operands:
  direct primitive, transformed primitive, plane clipper, nested CSG, composite,
  bounded finite primitive, and generic fallback
- precompute composed transforms and stable material/detail-owner policy in the
  baked records
- keep `environment/geometry` responsible only for primitive math and
  geometry-local detail calculation
- move render traversal policy and operand classification into
  `environment/scene` baked structures or render-side compiled helpers
- keep a generic fallback, but measure and reduce its use rather than letting
  it stay as the default hot path

Exit criterion:

- the full correctness gate passes under the `100000` metric rule and the
  blue-only heat-map exception rule
- `drums` does not regress
- fallback counters show fewer generic operand/object dispatches than Phase 1
- the next CSG-specific phases can target explicit baked variants instead of
  rediscovering shape patterns during tracing

## Phase 3: Keep The Single-Core Plane Intersection Specialization

Target:
`traceSingleCorePlaneIntersection` should use the baked `coreIndex` explicitly
instead of rediscovering or treating the core operand generically.

Why:
`drums` contains many CSG forms that look like a core curved primitive clipped
by planes. When the baked representation already knows which operand is the
core, using that index avoids repeated generic operand handling and reduces
unnecessary membership work. This is a small but reliable specialization
because it follows information produced by the scene bake step instead of
guessing from runtime geometry.

Required behavior:

- validate `coreIndex` once before using the specialization
- trace the core operand first
- use plane operands only as clipping / membership constraints
- preserve candidate ownership, material override, and local-point policy
- fall back to the generic path when the baked record is malformed or not a
  true single-core plane intersection

Expected result:
This phase should keep the already observed improvement from roughly `11.27s`
to roughly `11.12s` on `drums`, with no image change against the previous
scene-owned baked image and the same known `AE=71` against the older baked
baseline.

Exit criterion:

- optimized `drums` remains around the `11.1s` range
- previous-current image comparison remains exact
- older-baseline comparison remains within the known sparse `AE=71` pattern
- the full correctness gate passes under the `100000` metric rule and the
  blue-only heat-map exception rule

## Phase 4: Reduce `buildRaySegments` Work With Pattern-Specific Paths

Target:
`buildRaySegments` is the largest self-time hotspot in the scene-owned baked
branch. It should stop dominating `drums`.

Why:
The current path constructs full interval/segment state even for common CSG
forms where the final visible candidate can be decided with a narrower rule.
The older baked branch keeps more of this work in a direct Morgan-style
candidate path and is faster. However, previous blanket attempts to replace
interval logic with Morgan containment regressed, so this phase must not be a
global algorithm swap.

Proposed change:

- classify baked CSG nodes by exact shape pattern at bake time
- start only with the common `drums` pattern: one core primitive clipped by
  multiple planes
- produce candidate crossings from the core primitive and validate them against
  the plane set without building complete `RaySegments`
- still emit plane cap candidates when they are visible and semantically
  required
- keep the current full `RaySegments` path for nested, inverted, difference,
  ambiguous, or material-sensitive cases

Why this may work when earlier attempts failed:
Earlier experiments changed too much of the CSG algorithm at once. This phase
should exploit a narrow baked classification where the boolean form is known
and the core/clipping roles are explicit. The win must come from avoiding
segment allocation and merge work for that pattern, not from adding more
containment checks after the same amount of candidate work has already been
performed.

Exit criterion:

- `buildRaySegments` call count or self-time drops materially in `gprof`
- optimized `drums` improves beyond the Phase 3 range
- `cantelop`, `pacman`, `kscope`, and `pencil` remain stable enough to catch
  CSG classification mistakes
- the full correctness gate passes under the `100000` metric rule and the
  blue-only heat-map exception rule

Status update, 2026-07-02:

- Closed for the current `SingleCorePlaneIntersection` refinement cycle.
- The accepted work keeps the narrow single-core/plane specialization and
  guards it with `specializationValid`.
- Attempts to push the same specialization into first-hit traversal were
  rejected: exact image output, but slower wall-clock.
- The remaining Phase 4 hypothesis should not be retried by wrapping
  `traceFirstHit` or adding more candidate validation around planes. A future
  attempt must change the interval/candidate representation itself or target a
  different measured pattern.

## Phase 5: Reduce `traceOperandAllCrossings` Dispatch Volume

Target:
`traceOperandAllCrossings` is called about `150M` times in the current
instrumented `drums` run. That volume is too high for a scene representation
that is already baked.

Why:
The scene-owned baked layer currently carries a generic operand abstraction
deep into the hot loop. Each operand crossing can still pay repeated kind
checks, nested CSG checks, transform decisions, material-owner annotation
decisions, and scratch-routing decisions. The older baked path is less modular
but more direct, so it avoids much of this per-operand tax.

Proposed change:

- split baked operands into preclassified execution variants during scene bake
- use direct function paths or compact enum-dispatched loops for the common
  variants:
  non-transformed primitive, transformed primitive, nested CSG, plane clipper,
  and finite bounded primitive
- hoist invariant decisions out of `traceOperandAllCrossings`
- avoid rechecking transform, nested-index, and annotation capability on every
  ray when the baked operand already knows the answer
- add lightweight counters or profile labels so later `gprof` runs can
  distinguish generic fallback calls from specialized operand calls

Why:
This attacks the second-largest current hotspot without changing boolean
semantics. The expected benefit is lower dispatch and branch overhead while
leaving primitive math and candidate ordering unchanged.

Exit criterion:

- `traceOperandAllCrossings` call count or self-time falls in `gprof`
- the reduction does not simply move the same cost into another generic wrapper
- optimized `drums` improves or at least stays neutral while preparing Phase 4
  and Phase 7
- the full correctness gate passes under the `100000` metric rule and the
  blue-only heat-map exception rule

Status update, 2026-07-02:

- Closed for the first execution-kind pass.
- Accepted changes:
  - baked simple bodies and CSG operands now carry execution enums
  - direct annotated primitives, direct primitives, direct planes, transformed
    planes, transformed primitives, nested CSG, and transformed nested CSG are
    dispatched from those enums
  - direct and transformed planes use the scene-owned baked plane fields for
    intersection
- Current measured state after the accepted baked-plane change:
  `traceOperandAllCrossings` still reports `150,745,584` calls in `gprof`, and
  optimized `drums` measured `10.75`, `10.71`, `10.72` seconds with `AE=0`
  against the previous accepted image.
- Rejected changes:
  - direct plane-containment bypass in the single-core specialization
  - first-hit routing through `traceSingleCorePlaneIntersection`
  - first-hit plane-only local-queue bypass
- Do not mark more Phase 5 work as acceptable unless it reduces the
  `traceOperandAllCrossings` call count/self-time or clearly improves wall
  clock without increasing candidate/queue traffic.

## Phase 6: Avoid Generic Scratch And Pooling Optimizations

Target:
Keep scratch and queue work from growing, but do not spend another phase on
generic scratch ownership unless a profile proves the exact site dominates.

Why:
Queue-pool and scratch reuse were already partially improved, and the wall-time
gain was modest compared with the reduction in pool calls. Later attempts to
share more scratch from outer layers regressed. That indicates the remaining
gap is not mainly a pool-allocation problem; it is caused by building too much
CSG scaffolding and routing too often through generic traversal.

Allowed work:

- keep existing local scratch contexts that already proved useful
- make small, local cleanup changes if they reduce instructions at a measured
  hotspot
- add assertions or debug counters to catch stale scratch reuse when changing
  CSG traversal

Disallowed work unless a future profile changes the evidence:

- moving CSG scratch ownership to `BakedSimpleBodyTracing`
- heap-backed reusable local-ray scratch stacks
- generic queue-range transfer layers
- active-limit priority queues as nearest-hit heaps
- late candidate filtering after full CSG work has already been paid

Why this is a phase:
This phase is a guardrail. It prevents the next optimization cycle from
repeating changes that are plausible in isolation but already measured as
neutral or slower. Any scratch change must be justified by Phase 4 or Phase 5
and must remove work, not merely move ownership upward.

Exit criterion:

- no new generic scratch abstraction is introduced without a before/after
  `gprof` reason
- any scratch-related patch must be paired with a targeted reduction in
  `buildRaySegments`, `traceOperandAllCrossings`, or local-ray clones
- the full correctness gate passes under the `100000` metric rule and the
  blue-only heat-map exception rule

## Phase 7: Reduce `LocalIntersectionClone` Only In Narrow Hot Patterns

Target:
`RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)` is still called
about `85.5M` times in the current `drums` profile.

Why:
Local-ray cloning is expensive because it repeats ray/container setup in deep
CSG traversal. However, previous broad clone-reuse attempts regressed, so the
problem is not simply that construction happens on the stack. The useful win
must remove clone creation from a known hot pattern, not replace it with a
heavier scratch object.

Proposed change:

- first reduce clones in the same single-core plane-intersection and
  core-plus-clippers patterns targeted by Phase 3 and Phase 4
- keep one transformed ray per stable operand path only when the transform is
  actually needed
- avoid clone construction entirely for no-transform primitive operands
- prefer passing explicit local ray data or a narrow view object only inside the
  specialized pattern, not across the whole baked CSG API
- leave the generic CSG path unchanged until the specialized path is proven

Why this may work when the reusable scratch stack failed:
The failed attempt preserved the same logical amount of traversal and added
scratch lifetime management. This phase should instead eliminate clone sites by
choosing a more direct execution path for known baked patterns. Fewer clones
should be a consequence of doing less generic traversal, not a separate object
pooling technique.

Exit criterion:

- `LocalIntersectionClone` calls drop sharply for `drums`
- optimized `drums` improves beyond the Phase 3 range
- no extra heap-backed scratch stack appears on the hot path
- image comparison stays within the known sparse delta pattern
- the full correctness gate passes under the `100000` metric rule and the
  blue-only heat-map exception rule

Status update, 2026-07-02:

- Closed for the plane-specific clone-reduction pass.
- Accepted change: transformed plane candidates now use explicit transformed
  origin/direction plus baked plane intersection instead of constructing a
  `RayWithSegments::LocalIntersectionClone`.
- The optimized `drums` range improved to about `10.94` to `11.05` seconds
  before the direct-plane baked intersection refinement, and the final accepted
  baked-plane samples measured `10.75`, `10.71`, `10.72` seconds.
- The `gprof` counter for `LocalIntersectionClone` remains high
  (`85,492,814` calls), so the accepted change improved wall-clock but did not
  materially reduce the global clone count. The remaining clones are in
  transformed primitive/nested/composite paths.
- Do not retry broad clone reuse, first-hit specialization, or plane-only queue
  bypasses as Phase 7 work. A future Phase 7 attempt must remove clones from a
  measured transformed primitive or nested-Csg path without adding scratch
  lifetime management.

## Closed Phase Summary, 2026-07-02

| Phase | Status | Notes |
| --- | --- | --- |
| Phase 1 | Closed | Final baked-scene boundary is explicit before render initialization; fallback counters are visible. |
| Phase 2 | Closed | Baked records are execution-oriented through simple-body and CSG operand enums. |
| Phase 3 | Closed | `SingleCorePlaneIntersection` is retained and guarded by `specializationValid`. |
| Phase 4 | Closed for current cycle | First-hit/single-core refinements were exact but slower; do not repeat them. |
| Phase 5 | Closed for first execution-kind pass | Enum dispatch and baked plane direct paths are accepted; remaining `traceOperandAllCrossings` volume needs a deeper design. |
| Phase 6 | Closed as guardrail | Generic scratch/pooling remains disallowed unless a new profile proves a specific site dominates. |
| Phase 7 | Closed for plane-specific pass | Transformed/direct plane baked intersection is accepted; remaining clone count must be attacked in transformed primitive or nested paths. |

## Phase 8: Transformed Primitive Clone Reduction

Target:
Reduce the remaining `traceOperandAllCrossings` and
`RayWithSegments::LocalIntersectionClone` cost in transformed primitive or
nested-Csg paths, without repeating the rejected first-hit, scratch ownership,
or plane-containment variants.

Why:
After the plane-specific pass, `LocalIntersectionClone` still reports
`85,492,814` calls in `drums`, and `traceOperandAllCrossings` still reports
`150,745,584` calls. The accepted plane work improved optimized wall-clock but
did not materially move the global clone counter, which means the next useful
attempt must remove clones from transformed primitive or nested-Csg execution.

Allowed first attempt:

- classify a specific transformed primitive kind at bake time
- start with transformed `Quadric`, because `drums` remains quadric-heavy
- avoid dynamic type checks in the hot path by storing a dedicated execution
  kind
- avoid `RayWithSegments::LocalIntersectionClone` and local scratch queues for
  that specific transformed primitive
- keep nested CSG and non-quadric transformed primitives on the accepted
  generic path

Disallowed repeats:

- routing first-hit through `traceSingleCorePlaneIntersection`
- plane-only shortcuts inside `traceFirstHitByIntersectionMembership`
- direct plane-containment bypass inside `SingleCorePlaneIntersection`
- reusable local-ray scratch stacks or moving scratch ownership upward
- broad primitive-type `dynamic_cast` direct-write logic in the hot path

Exit criterion:

- optimized `drums` improves or remains neutral with `AE=0` against the
  accepted baked-plane image
- a `gprof` run shows either lower `LocalIntersectionClone` calls or lower
  `traceOperandAllCrossings` self-time
- the full correctness gate passes before the phase is marked accepted
- if the first attempt is exact but slower, document it here and close the
  attempt as rejected

Status update, 2026-07-02:

- Accepted for transformed `Quadric` CSG operands.
- Added `TransformedQuadric` as a baked CSG operand execution kind, classified
  at scene bake time so the hot path avoids primitive-type `dynamic_cast`.
- `BakedCsgTracing` now intersects transformed quadric operands from explicit
  local origin/direction data and emits transformed candidates directly,
  avoiding `RayWithSegments::LocalIntersectionClone` and the local scratch
  queue for that specific path.
- Optimized `drums` measured `10.74`, `10.57`, `10.58` seconds with `AE=0`
  against the accepted baked-plane image.
- `gprof` changed as follows:
  - `LocalIntersectionClone`: `85,492,814` -> `75,039,972` calls
  - `PriorityQueuePool::pop`: `93,802,877` -> `83,359,020` calls
  - `PriorityQueuePool::push`: `52,872,523` -> `45,626,731` calls
  - `RayWithSegments::makeRay`: `41,178,805` -> `30,725,963` calls
  - instrumented `drums`: about `29.29s` -> `28.69s`
- `traceOperandAllCrossings` call count remains `150,745,584`, because this
  phase reduces work inside the dispatcher rather than bypassing the dispatcher.
- Full correctness gate passed:
  `./scripts/clean.sh`,
  `./scripts/compile.sh -DCMAKE_CXX_FLAGS= -DCMAKE_EXE_LINKER_FLAGS=`,
  `./scripts/renderAll.sh` (`175s`),
  `./scripts/testAgainstGoldenImages.sh`.

## Phase 9: Transformed Nested CSG Clone Reduction

Target:
Reduce `RayWithSegments::LocalIntersectionClone` calls that remain in
`TransformedNestedCsg` operands.

Why:
After Phase 8, `LocalIntersectionClone` dropped from `85,492,814` to
`75,039,972` calls, but transformed nested CSG and transformed composite/simple
body paths still dominate the residual clone count. `TransformedNestedCsg` is
the next CSG-owned target because it still builds a local cloned ray before
recursing into `traceAllCrossingsWithScratch`.

Allowed first attempts:

- classify transformed nested CSG sub-patterns at bake time when that avoids
  hot-path type checks
- specialize only nested CSG forms whose child specialization is already known
  and image-stable
- pass explicit local origin/direction into a narrow specialized nested path
  only when that path can avoid constructing a full local ray
- keep generic transformed nested CSG on the accepted clone-based path

Disallowed repeats:

- first-hit shortcuts around `traceFirstHitByIntersectionMembership`
- plane-only first-hit queue bypasses
- direct plane-containment bypasses in `SingleCorePlaneIntersection`
- heap-backed local-ray scratch stacks
- moving scratch ownership out to `BakedSimpleBodyTracing`

Exit criterion:

- optimized `drums` improves or remains neutral with `AE=0` against the Phase 8
  accepted image
- `gprof` shows lower `LocalIntersectionClone` calls or a targeted reduction
  in work under transformed nested traversal
- full correctness gate passes before marking the phase accepted
- exact but slower attempts must be documented here and reverted

Status update, 2026-07-02:

- First attempt rejected and reverted.
- Attempted pattern:
  transformed nested CSG whose child was `SingleCorePlaneIntersection` with a
  transformed-quadric core and plane clippers. The implementation passed
  explicit nested origin/direction and avoided the parent local-ray clone only
  for that narrow shape.
- Image stability was exact (`AE=0`) against the Phase 8 accepted image.
- Optimized `drums` measured `11.00`, `10.66`, `10.67` seconds after removing
  an initial local-queue variant; this was at best neutral and less stable than
  Phase 8's `10.74`, `10.57`, `10.58` seconds.
- `gprof` did not show the desired reduction:
  `LocalIntersectionClone` stayed at `75,039,972` calls and
  `traceOperandAllCrossings` stayed at `150,745,584` calls. Internal work moved
  around instead, including increased `ColorOperations::clipColor` samples.
- Do not retry this explicit nested single-core/plane variant unless a future
  profile proves it is actually reached often enough to reduce clone count.

Status update, 2026-07-02, different Phase 9 approach:

- Second attempt rejected and reverted.
- Attempted pattern:
  add a dedicated transformed-local-ray constructor so transformed operands
  could build the local ray with final origin/direction in one step instead of
  cloning and then calling `setOrigin`, `setDirection`, and
  `setQuadricConstantsCached(false)`.
- Image stability was exact (`AE=0`) against the Phase 8 accepted image.
- Optimized `drums` measured `10.71`, `10.70`, `10.67` seconds, which is not a
  clear improvement over Phase 8's `10.74`, `10.57`, `10.58` seconds.
- `gprof` showed the change was only a cost split:
  `LocalIntersectionClone` dropped to `30,173,244` calls, but the new
  transformed constructor added `44,866,728` calls. The sum remains
  `75,039,972`, with `PriorityQueuePool::pop`, `PriorityQueuePool::push`,
  `RayWithSegments::makeRay`, and `traceOperandAllCrossings` unchanged from
  Phase 8.
- Do not retry constructor-only transformed-local-ray splitting; future Phase 9
  work must remove transformed nested traversal work, not rename the clone path.

## Phase 10: Internal `traceOperandAllCrossings` Work Reduction

Target:
Reduce work performed inside `traceOperandAllCrossings` after the dispatcher
call count itself has proven hard to reduce.

Why:
Phase 8 reduced local ray clones and queue-pool traffic without reducing the
`traceOperandAllCrossings` call count (`150,745,584`). The remaining useful
work is likely inside the dispatcher: repeated common setup, candidate
construction, queue traffic, and per-kind branch cost.

Allowed work:

- split hot execution kinds into narrower helpers if measurements show lower
  self-time without simply moving cost to another wrapper
- reduce candidate construction or queue traffic only for a measured operand
  kind
- hoist invariant per-kind setup that is currently repeated inside the hot
  function
- keep material/detail attribution exact

Disallowed repeats:

- broad primitive-type `dynamic_cast` direct-write logic
- generic early first-hit shortcuts
- generic scratch/pooling abstractions
- late filtering after full candidate generation
- blanket Morgan containment rewrites

Exit criterion:

- `traceOperandAllCrossings` self-time falls in `gprof` or optimized `drums`
  improves with identical image output
- no increase in primitive-test counts or queue traffic that cancels the win
- full correctness gate passes before marking the phase accepted

## Historical Execution Order

This was the intended execution order for the now-closed cycle:

1. Phase 1 established the final baked scene boundary after parse/postprocess
   and before render.
2. Phase 2 made baked records execution-oriented and measured remaining
   original-scene fallback.
3. Phase 3 kept the already proven `coreIndex` specialization, guarded by the
   stronger baked-scene boundary.
4. Phase 4 attacked `buildRaySegments` for the core-plus-plane-clippers pattern
   and closed the first-hit/single-core variants after measured regressions.
5. Phase 5 reduced operand dispatch through execution-kind paths and closed the
   first execution-kind pass after the baked-plane refinement.
6. Phase 7 removed local-ray clones only from the proven plane-specific path
   and closed broader first-hit/plane shortcut attempts as regressions.
7. Phase 6 applied continuously as a constraint; scratch ownership remains a
   guardrail, not a standalone optimization target.

The main performance hypothesis is that the current branch can approach the
older baked baseline only if the scene-owned baked representation becomes the
actual render input. Direct specialized execution for common CSG shapes remains
necessary, but it should be built on top of a finalized baked scene rather than
on a thin cache that repeatedly re-enters parsed scene objects. Generic caching,
pooling, and traversal wrappers have already shown diminishing returns.
