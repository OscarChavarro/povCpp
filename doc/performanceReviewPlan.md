# Performance Improvement Plan

## Scope

This document now focuses on the active performance-improvement plan for the
current worktree, not on the historical full sweep across old commit pairs.

- Comparison baseline commit:
  `4af1a75e5b7356600bec34e12a4882560994a058`
- Candidate under measurement:
  the current uncommitted worktree
- Fixed benchmark scenes for every optimization step:
  `level2/spline`
  `level3/ntreal/ntreal`
  `level3/piece3/piece3`
  `level2/iortest`
  `level1/shapes2`
- Primary timing mode:
  `320x200`, same lightweight profiling mode already used for the targeted
  `gprof` passes

## Benchmark Gate

Every change intended to improve performance must pass both gates:

1. Runtime gate
   The current worktree must be measured against
   `4af1a75e5b7356600bec34e12a4882560994a058` on the five fixed scenes.
2. Image-stability gate
   Comparative heatmaps for those scenes must remain sparse and low-structure.
   Broad, coherent, or scene-wide differences block the change even if it is
   faster.

The practical rule is simple: performance wins are acceptable only when the
heatmaps still look like small local perturbations, not like a changed render
algorithm.

## Baseline Reference

These are the current reference times from the selected baseline commit at
`320x200`:

| Scene | Baseline commit | Time (s) |
| --- | --- | ---: |
| `level2/spline` | `4af1a75` | 1.989 |
| `level3/ntreal/ntreal` | `4af1a75` | 1.292 |
| `level3/piece3/piece3` | `4af1a75` | 4.112 |
| `level2/iortest` | `4af1a75` | 5.316 |
| `level1/shapes2` | `4af1a75` | 0.382 |

## Current Worktree Check

After introducing the first tracing-scene cache and top-level `AABB` culling
for bounded objects, the current worktree measured as follows on 2026-06-30:

| Scene | Baseline 4af1a75 (s) | Current worktree (s) | Factor |
| --- | ---: | ---: | ---: |
| `level2/spline` | 1.989 | 2.33 | 1.171 |
| `level3/ntreal/ntreal` | 1.292 | 0.94 | 0.727 |
| `level3/piece3/piece3` | 4.112 | 4.64 | 1.128 |
| `level2/iortest` | 5.316 | 6.31 | 1.187 |
| `level1/shapes2` | 0.382 | 0.30 | 0.785 |

Reading:

- The top-level broad phase already helps scenes where many bounded objects can
  be rejected early.
- It does not solve the worst structural overhead yet, especially in the
  `spline` / `piece3` / `iortest` family.
- That confirms the next high-value work should move from scene entry into the
  first-hit path and the CSG/runtime-crossings path.

### Rejected experiment: early first-hit fast path

An additional experiment on 2026-06-30 tried to bypass
`doIntersectionForAllRayCrossings` inside `SimpleBody::doIntersectionFirstHit`
for a subset of primitives.

Result:

- the image gate stayed correct on the spot-checked scenes
- but the 5-scene timing panel regressed badly, especially on
  `level3/ntreal/ntreal`, `level3/piece3/piece3`, and `level2/iortest`

Decision:

- do not keep that change in the worktree
- revisit first-hit specialization only after the scene/baking work has reduced
  the higher-level composite and CSG overhead

### Rejected experiment: top-level BVH / eye-distance ordering

Two more scene-layer traversal experiments were measured on 2026-06-30 after
the initial broad-phase cache:

- a top-level BVH over bounded objects
- a cheaper linear pass with bounded objects preordered by centroid distance to
  the camera eye

Result:

- both preserved image correctness on the spot-checked scenes
- both regressed the 5-scene timing panel relative to the simpler linear
  bounded-object broad phase already in the worktree

Decision:

- do not keep either variant in the worktree
- keep the simpler linear bounded-object culling as the current best scene-entry
  optimization
- move the next optimization effort back toward compiled scene representation
  and deeper tracing-path structure, not more top-level traversal machinery

### Rejected experiment: direct first-hit path for CSG unions

Another experiment on 2026-06-30 tried two related shortcuts:

- routing baked no-clipping objects directly through `Geometry::doIntersectionFirstHit`
- specializing `ConstructiveSolidGeometryByRaySegment` unions for an outside-ray
  nearest-hit fast path

Result:

- both variants regressed the benchmark panel badly
- `level3/ntreal/ntreal` in particular became much slower than the already
  improved compiled-scene branch

Decision:

- do not keep either shortcut in the worktree
- keep the compiled-scene and shadow-culling gains, but leave CSG first-hit
  specialization for a later, more explicit design pass

### Rejected experiment: shadow query with late `minT` / `maxT` filtering

An experiment on 2026-06-30 tried to add a baked shadow query that passed the
light segment range (`SHADOW_TOLERANCE` to `lightSourceDepth`) down into
`BakedSimpleBodyTracing` / `BakedCsgTracing`.

Result:

- `level2/iortest` regressed from roughly `6.25s` to `6.50s` / `6.53s`
- applying the range after candidates were already generated did not reduce
  primitive intersection counts
- adding the range checks and extra dispatch made the hot CSG/shadow path
  slower even when the rendered statistics and images stayed stable

Decision:

- do not keep a generic `traceAllCrossingsBefore(minT, maxT)` layer
- do not filter CSG candidates by range after full Morgan membership work has
  already been paid
- the next shadow-query attempt must change candidate production order or
  consumption order, not merely discard already-built candidates

Follow-up kept in the worktree:

- `DirectLightShader` now has a dedicated exact shadow-object query helper that
  traces one compiled object, consumes its candidate queue in `t` order, applies
  the existing light-segment / `no_shadow` tests, and calls `ShadowShader` until
  the light is blocked
- this is intentionally a behavioral refactor, not the rejected late-filter
  optimization
- when `WITH_FILTERED_SHADOWS` is disabled, the helper can return blocked on
  the first valid shadow-casting candidate without entering surface shading;
  with filtered shadows enabled (the default), it preserves the previous
  transparency/filtering path exactly
- measured `level2/iortest` at `320x200`: neutral at about `6.28s`
- `level3/kscope` and `level3/pencil` remained exact (`AE=0`, `RMSE=0`)

### Rejected experiment: containment `AABB` reject for baked CSG operands

On 2026-06-30, another experiment tried to reintroduce a containment-side
`AABB` early-out inside `BakedCsgTracing::containmentTestOperand`, but only for
operand kinds whose bounds looked safe to use.

Result:

- the optimization stayed exact on `level3/kscope`, `level3/pencil`, and
  `level1/cantelop`
- but `level2/pacman` still diverged (`AE=1`) even after tightening the check
  with `distanceTolerance` and `Config::SMALL_TOLERANCE`
- because the change had not yet demonstrated a measured panel win, the image
  gate failure was enough to reject it

Decision:

- do not keep any containment-side baked `AABB` reject for CSG operands unless
  exactness is proven on `cantelop` and `pacman`, not only on `kscope` and
  `pencil`
- treat operand containment as more fragile than operand crossing generation;
  one-pixel differences here can still indicate a real classification error

## What The Current Code Review Suggests

The current code still pays a structural tracing cost that is larger than the
actual primitive-intersection cost for the regression-heavy scenes.

### 1. `Scene` is only partially tracing-aware

`src/environment/scene/Scene.h` and `src/environment/scene/Scene.cpp` now do
more than hold raw parsed objects and lights: the scene builds a small tracing
cache that splits bounded and unbounded top-level objects and stores world-space
`AABB`s for the bounded ones.

What is still missing:

- there is no tracing-oriented compilation phase
- there is no flattened traversal representation
- there is no baked immutable scene graph such as `BakedScene` / `BakedSolid`

Implication:
the current worktree already avoids some useless top-level intersection calls,
but every ray still traverses the original `SimpleBody*` object graph rather
than a compiled tracing representation.

### 2. Top-level tracing now has a simple broad phase, but traversal is still object-by-object

`RenderEngine::trace` in `src/render/RenderEngine.cpp` no longer starts with a
fully naive full-scene scan: it first culls bounded top-level objects against
their cached `AABB`s, then calls `doIntersectionFirstHit` on the survivors and
on all unbounded objects.

Implication:
the current renderer already has a useful top-level broad phase, but the hot
path still walks object-by-object and still enters the old `SimpleBody`
intersection stack instead of a denser compiled traversal structure.

### 3. First-hit queries still go through all-crossings machinery

`SimpleBody::doIntersectionFirstHit` allocates a queue and delegates to
`doIntersectionForAllRayCrossings`, even when the caller only needs the nearest
hit.

Implication:
the hot path still pays for queue traffic, candidate mutation, and crossing
sorting that are unnecessary for many rays.

### 4. Local-space ray clones are still everywhere

`SimpleBody`, `Composite`, and `CsgOperand` all build
`RayWithSegments(LocalIntersectionClone, ...)` instances on hot paths. The
clone constructor has already been optimized, but the representation still
depends on repeated ray cloning and repeated matrix application.

Implication:
the current improvements reduced cost, but they did not change the underlying
execution model.

### 5. CSG still creates a lot of transient work

`ConstructiveSolidGeometryByRaySegment.cpp` still:

- allocates a scratch priority queue per child
- builds temporary `RaySegments`
- copies candidates while polling and re-emitting crossings
- performs child-by-child merge logic even when the scene is stable and parsed
  only once

Implication:
the current CSG path remains runtime-oriented instead of precompiled.

## Main Direction: Reintroduce Baking In `environment.scene`

The most coherent direction is still to reintroduce a baked tracing
representation, but to do it in `environment.scene`, not by restoring mutable
transform baking inside `Geometry`.

### Proposed model

Add a scene-compilation step after parsing, for example:

- `Scene::bakeForTracing()`
- a compiled `BakedScene`
- immutable tracing nodes such as `BakedSolid`, `BakedComposite`, and
  `BakedCsgNode`

### What must be baked

Each baked tracing node should precompute at least:

- composed object-to-world transform
- composed world-to-object inverse
- geometry-layer transform folded into the final tracing representation
- world-space `AABB`
- stable flags such as `no_shadow`, `has_bounds`, `has_clipping`
- direct references to material, texture, and color state

### Why this is the right layer

This keeps the current architecture intact:

- `io/pov/...` keeps parsing scene syntax
- `environment/geometry/...` keeps primitive math
- `environment/scene/...` becomes the compilation boundary between parsed scene
  objects and tracing objects
- `render/...` consumes a denser tracing structure instead of the raw parsed
  tree

That matches the current ownership split better than pushing mutable baking back
down into `Geometry`.

## Concrete Plan

### Phase 0: Keep the benchmark harness fixed

- Keep the five scenes above as the permanent regression panel.
- Compare every performance branch against
  `4af1a75e5b7356600bec34e12a4882560994a058`.
- Save timing deltas and comparative heatmaps for each iteration.

Status on 2026-06-30:
still active; this remains the non-negotiable measurement gate for every next
step.

Deliverable:
a repeatable before/after table per change.

### Phase 1: Introduce scene compilation without changing behavior

- Extend `Scene` with a compiled representation stored alongside the parsed
  objects.
- Build the compiled scene once after parsing, before rendering starts.
- Keep compiled nodes behaviorally equivalent to the current `SimpleBody` /
  `Composite` / `CsgOperand` tree.

Status on 2026-06-30:
in progress in the current worktree. `Scene` now owns compiled tracing records
beside the parsed object array, but the compiled layer is still a thin wrapper
around the current `SimpleBody` / `Composite` / `CsgOperand` tree rather than a
fully flattened baked scene graph.

Updated status on 2026-06-30:
complete as a compilation boundary. `Scene::CompiledTracingScene` now owns:

- top-level `CompiledTracingObject` records
- baked flat `SimpleBody` records
- baked `Composite` records
- baked `bounding` / `clipping` helper subgraphs for those records
- top-level bounded/unbounded partitions
- top-level shadow-caster partitions

Important distinction:
this phase is now ready as an infrastructure phase. The baked graph is
structurally independent from the legacy `SimpleBody` / `Composite` tree during
tracing: render-time traversal no longer fetches child/bounding/clipping edges
from legacy containers. It is still not a license to delete legacy behavior
yet, because CSG and any not-yet-baked case may still use legacy algorithms as
leaf fallbacks.

Deliverable:
same images, same traversal semantics, but a dedicated tracing representation is
available.

### Phase 2: Flatten simple objects first

- Compile plain `SimpleBody` instances into dense `BakedSolid` records.
- Precompose owner and geometry-layer transforms once.
- Precompute world-space bounds once.
- Remove repeated transform composition from the ray hot path for non-CSG
  primitives.

Expected benefit:
this should help all five reference scenes and is the safest first step for the
heatmap gate.

Status on 2026-06-30:
partially completed in the current worktree for bakeable flat `SimpleBody`
entries. The renderer now has a direct baked tracing helper that reuses
precomposed owner/geometry transforms, cached bounds, and direct
geometry/material/color references for those objects, while composite and CSG
paths still fall back to the legacy runtime traversal.

Updated status on 2026-06-30:
partially completed and active. Flat bakeable `SimpleBody` entries use
`BakedSimpleBodyTracing` for primary rays and direct-light shadow rays.
`Composite` entries are also compiled into `BakedComposite` containers, but
their semantics are deliberately conservative:

- composite children are traversed in the exact legacy order
- child records are not internally reordered by bounded/unbounded role
- `SimpleBody` geometries that are CSG now traverse a baked CSG graph instead
  of reading operand arrays from legacy geometry objects at render time
- bounding/clipping helper subgraphs are compiled into the baked graph and
  traversed through `CompiledTracingObject`

Do not mark Phase 2 complete until CSG and bounding/clipping subgraphs are also
represented in a baked form, or until the plan explicitly decides to leave
those subgraphs as legacy islands.

### Phase 3: Add a top-level broad phase

- Build a scene-level acceleration structure from baked world-space bounds.
- Start with something simple and inspectable:
  a flat array plus broad-phase culling, uniform bins, or a small BVH.
- Use `SimpleBody::getAABB()` / baked bounds data instead of starting every ray
  with a full linear scan.

Status on 2026-06-30:
partially completed in the current worktree. `Scene::buildTracingCache()` now
separates bounded and unbounded objects and `RenderEngine::trace` culls bounded
entries by cached world-space `AABB`, but the more ambitious BVH / ordering
variants tested so far regressed the benchmark panel.

Updated status on 2026-06-30:
partially completed and active at the top level only. Primary rays and shadow
rays both consume the compiled top-level partitions. The top-level broad phase
is acceptable; internal broad-phase culling inside `BakedCompositeTracing` is
not yet acceptable because a previous attempt changed object ordering and broke
golden scenes.

Rule:
any future composite-child culling must preserve the same observable candidate
ordering as `Composite::doIntersectionForAllRayCrossings`. If that cannot be
proven locally, do not add the optimization.

Status on 2026-07-01:
complete for the current safe broad-phase scope. The renderer now has:

- top-level bounded/unbounded partitions for primary rays
- top-level bounded/unbounded shadow-caster partitions for shadow rays
- order-preserving child `AABB` culling inside `BakedCompositeTracing` for
  composites without their own object transform
- child `AABB` pruning in baked composite containment tests

The internal composite ray culling deliberately does not run for transformed
composites, because comparing transformed composite-local rays against child
bounds previously broke `level3/kscope`. Do not reintroduce transformed
composite child culling unless the bounds are explicitly represented in the
same space as the ray and `kscope`/`pencil` stay exact.

Expected benefit:
this directly attacks the current `RenderEngine::trace` bottleneck.

### Phase 4: Replace generic first-hit traversal on the common path

- Add a dedicated first-hit API for baked simple solids.
- Avoid building full crossing queues when the caller only wants the nearest
  visible hit.
- Keep full all-crossings logic for CSG, clipping-heavy objects, and other
  cases that truly need it.

Expected benefit:
less `PriorityQueuePool` traffic and less candidate churn on primary and shadow
rays.

Status on 2026-07-01:
complete for the common safe path. `Geometry` now exposes
`doIntersectionFirstHitNoQueue`, and `BakedSimpleBodyTracing::traceFirstHit`
uses it for flat baked simple bodies that do not have CSG, bounding helpers, or
clipping helpers. The first covered primitives are `Quadric`, `Sphere`, `Box`,
and `InfinitePlane`.

Important constraint:
do not widen this shortcut to CSG, clipping-heavy objects, or bodies with
bounding helper shapes unless the change proves exactness against the full
golden-image gate. A previous broad shortcut broke scenes such as `car` by
changing primitive hit selection; direct first-hit implementations must preserve
nearest-root selection and material override behavior.

### Phase 5: Bake CSG operands and transforms

- Compile `CsgOperand` trees into baked operand nodes with composed transforms,
  cached bounds, and stable child arrays.
- Avoid reconstructing child-local rays and temporary segment lists more than
  necessary.
- Special-case trivial forms discovered at bake time, for example unions that
  can stay as flat child batches.

Expected benefit:
this is the step most likely to recover the worst losses in `spline`,
`ntreal`, `piece3`, and `iortest`.

Status on 2026-06-30:
started as structure, not as optimization. `Scene` now compiles CSG operand
graphs into baked records and `BakedSimpleBodyTracing` routes CSG geometries
through that baked graph for both crossings and containment, preserving the
Morgan and Roth variants. The implementation is intentionally conservative:
it still mirrors the legacy algorithms' queue-heavy behavior and still uses
legacy `CsgOperand` objects only as semantic detail owners for normal/material
reconstruction.

Accepted follow-up on 2026-06-30:

- `BakedCsgTracing::traceOperandAllCrossings` now has a direct-to-destination
  fast path for the safe case `no transform` + `no nested baked CSG` +
  non-plane primitive geometry
- that path lets the primitive intersect directly into the destination queue
  and then annotates only the fresh owner-less candidates, avoiding the extra
  scratch queue and copy/re-offer loop
- measured `level2/iortest` at `320x200`: about `6.28s` -> `6.04s`
- `level3/kscope` and `level3/pencil` remained exact (`AE=0`, `RMSE=0`)

Constraint:

- do not extend that fast path to transformed operands or nested baked CSG
  operands without a separate proof that point-space and owner annotation stay
  equivalent

Status on 2026-07-01:
complete as the baked CSG phase. CSG operand trees are compiled into baked
operand nodes with stable arrays, composed transforms, cached bounds, nested
baked CSG references, conservative specializations, and active baked traversal
for both crossings and containment. The accepted specializations are:

- direct-to-destination tracing for safe primitive operands
- top-level plane unions
- pairwise-disjoint finite unions
- single-core plane intersections
- bounded containment pruning for pairwise-disjoint finite unions

Remaining CSG performance work should be treated as post-Phase-5 scaffolding or
new targeted optimization, not as missing bake infrastructure. Do not remove
the legacy `CsgOperand` detail-owner pointers yet; they are still part of
normal/material reconstruction semantics even though traversal no longer reads
operand arrays from legacy CSG objects at render time.

### Phase 6: Only then optimize the remaining runtime scaffolding

- Revisit `RayWithSegments` scratch usage.
- Revisit `PriorityQueuePool` sizing and ownership.
- Revisit candidate copying and temporary per-ray containers.

Expected benefit:
smaller but still meaningful wins after the tracing representation has been
fixed.

Status on 2026-07-01:
complete for the current safe scaffolding pass. The accepted change removes an
unconditional `RayWithSegments::LocalIntersectionClone` from
`BakedCompositeTracing::traceAllCrossings`; composite-local clones are now built
only for composites with an object transform. This keeps candidate order and
all intersection semantics unchanged while reducing per-composite scratch work.

Rejected/blocked follow-ups:

- do not use `PriorityQueuePool::pop(1)` or other active-limit tricks as a
  nearest-hit queue. The current `PriorityQueue` active limit rejects later
  insertions without comparing priority, so it is not a top-k heap.
- do not direct-write baked simple-body primitive hits into a shared destination
  queue unless fresh candidates can be identified without scanning/reannotating
  unrelated heap entries.
- keep transformed-composite child culling out of this phase; it belongs to a
  separately proven bounds-space change because the broad variant broke
  `level3/kscope`.

### Phase 7: Rebaseline CSG-heavy regressions against the older baked branch

The five-scene panel is now green, but `level3/drums2/drums.pov` exposed a
different regression class: the current scene-owned baked layer is slower than
the older baked branch on CSG-heavy content.

Measured on 2026-07-01 at `320x200`:

| Commit | Meaning | Runs (s) | Mean (s) | Factor |
| --- | --- | ---: | ---: | ---: |
| `4af1a75e5b7356600bec34e12a4882560994a058` | baseline with baked scheme | `5.13`, `5.13`, `5.11` | 5.12 | 1.00 |
| `acb6da387844b0aa39f1dd047b8fbd340d35f5ae` | baked extracted to `Scene` layer | `11.86`, `11.89`, `11.94` | 11.90 | 2.32 |

The `320x200` image comparison between the two commits was low but nonzero:
`AE=71`, `RMSE=344.851 (0.00526208)`.

`gprof` runs with `-pg`:

| Commit | Instrumented time (s) | Profile file |
| --- | ---: | --- |
| `4af1a75` | 12.06 | `/tmp/povCpp-drums-4af1a75/output/drums-gate/gprof-drums-4af1a75.txt` |
| `acb6da3` | 32.51 | `/tmp/povCpp-drums-acb6da3/output/drums-gate/gprof-drums-acb6da3.txt` |

Key profile reading:

- the current branch does fewer quadric primitive tests
  (`41.2M` vs. `68.8M`) but is still much slower
- the lost time moved into baked CSG scaffolding:
  `buildRaySegments`, `traceOperandAllCrossings`, `BakedCsgTracing`, queue
  pool traffic, and local ray clones
- current `drums` hot counts include about `176M` `PriorityQueuePool::pop`,
  `176M` `PriorityQueuePool::push`, `85.5M`
  `RayWithSegments::LocalIntersectionClone`, `136.3M`
  `traceOperandAllCrossings`, and `52.1M` `buildRaySegments`
- therefore this is not primarily a primitive-intersection problem; it is a
  runtime scaffolding problem inside the scene-owned baked CSG layer

Exit criterion:
keep `drums` in the active performance panel until the current branch is within
roughly `20%` of the older baked branch at `320x200`, or until a documented
semantic reason explains why the older baked branch is not a valid target.

### Phase 8: Remove per-operand CSG queue churn

Target:
`PriorityQueuePool::pop` / `push` are hot only because baked CSG allocates and
returns scratch queues per operand or per nested traversal. `drums` shows about
`176M` calls in each direction on the current branch.

Plan:

- introduce a CSG traversal scratch context owned by the outer CSG trace call
- pass reusable operand queues down through `BakedCsgTracing` instead of
  popping/pushing for every operand
- preserve the current candidate ordering and heap semantics; do not replace
  queues with vectors until the exact polling order is proven equivalent
- measure first on `drums` and then on the existing five-scene panel

Risks:

- queue reuse can leak stale candidates if every path does not clear before
  reuse
- recursive/nested CSG must either borrow distinct scratch slots or use an
  explicit stack discipline
- do not use `PriorityQueuePool::pop(1)` as a nearest-hit optimization; the
  active limit is not a top-k heap

Exit criterion:
`drums` must reduce queue-pool calls materially in `gprof`, while
`kscope`/`pencil` remain exact and the full golden-image gate keeps the
accepted AE pattern.

Result on 2026-07-01:

- implemented an explicit `CsgScratchContext` shared by the outer baked CSG
  traversal
- scratch queues are borrowed lazily and reused down nested CSG traversal; the
  fallback still returns overflow queues to the existing pool
- `drums` at `320x200` improved only modestly, from the previous current-branch
  mean `11.90s` to `11.64s`
- `gprof` queue-pool traffic dropped materially:
  `PriorityQueuePool::pop` from about `176.0M` to `101.0M`, and
  `PriorityQueuePool::push` from about `176.1M` to `45.6M`
- the remaining top costs are now `buildRaySegments`,
  `traceOperandAllCrossings`, containment checks, and local ray clones

Decision:
Phase 8 is complete for the queue-churn pass. The small wall-clock gain means
the `drums` gap is not primarily solved by pool reuse; continue with Phase 9
and Phase 10 rather than adding more queue-pool micro-optimizations.

### Phase 9: Pretransform simple CSG operands at bake time

Target:
`drums` spends substantial time cloning local rays inside baked CSG. The
current branch reported about `85.5M`
`RayWithSegments::LocalIntersectionClone` calls in the instrumented run.

Plan:

- add baked operand variants for simple primitive operands whose transform can
  be folded into the primitive once at scene-bake time
- start with `InfinitePlane` and `Quadric`, because `drums` is dominated by
  planes and quadrics
- keep material/detail-owner attribution unchanged; only the geometric operand
  representation should change
- preserve the original legacy `CsgOperand*` owner pointer for
  `doExtraInformation` until a separate semantic-owner replacement exists

Risks:

- transformed plane/quadrics are sensitive to point-space conventions used by
  containment, material coordinates, and normal reconstruction
- folded transforms must match the current `localToObject` /
  `objectToLocal` direction exactly
- one-pixel CSG containment errors are meaningful; `cantelop`, `pacman`,
  `kscope`, and `pencil` remain mandatory guards

Exit criterion:
`RayWithSegments::LocalIntersectionClone` should drop sharply in the `drums`
profile without introducing broad heatmap structure.

Rejected experiment on 2026-07-01:

- tried conservative pretransformed baked operands for transformed
  `InfinitePlane` and `Quadric` CSG operands
- the original `CsgOperand` stayed as detail owner, so the experiment preserved
  legacy normal/material ownership in the local spot checks
- `drums` at `320x200` only improved marginally (`11.58`, `11.68`, `11.61`,
  mean about `11.62s` vs. Phase 8 mean about `11.64s`)
- `gprof` showed `RayWithSegments::LocalIntersectionClone` dropping only from
  about `85.5M` to `81.1M`, not enough to satisfy the phase criterion
- the full golden-image gate failed outside the accepted AE pattern:
  `level3/snack AE=209113`, `level3/wg5 AE=518549`,
  `level3/piece1 AE=42176`, plus additional small new diffs

Decision:
do not keep this pretransformed plane/quadric variant. Phase 9 remains pending
unless a safer transformed-operand strategy can prove full-suite image stability
and a much larger clone-count reduction. The next practical optimization path
is likely Phase 10 because `buildRaySegments` remains the dominant `drums`
cost.

### Phase 10: Specialize RaySegments only where the boolean form needs it

Target:
The current branch pays `buildRaySegments` heavily on `drums`
(`52.1M` calls in the instrumented run). The older branch keeps more work in
the direct Morgan-style CSG path and is faster for this scene.

Plan:

- classify baked CSG nodes by algorithmic need, not only by source geometry
  class
- for Morgan-style intersections/unions where direct candidate testing is
  equivalent, route through a Morgan-equivalent baked path instead of building
  full `RaySegments`
- keep the existing Roth/ray-segment algorithm only for nodes that truly need
  interval merging
- add small counters or profile labels so future `gprof` runs distinguish
  Morgan-equivalent CSG from full segment-building CSG

Risks:

- boolean difference and nested/inverted operands can require full interval
  semantics
- direct candidate testing must preserve material/owner attribution and
  candidate order
- the shortcut must be disabled for cases where initial-inside classification
  changes visible crossings

Exit criterion:
`buildRaySegments` must no longer dominate `drums`; exactness must hold on CSG
semantic guards and the full golden-image gate.

Rejected experiments on 2026-07-01:

- routed `RaySegments` intersections through the baked Morgan-intersection
  path; the fast panel and `kscope`/`pencil` stayed exact, but `drums` regressed
  to `11.86`, `11.89`, `12.17` seconds because the extra containment filtering
  outweighed the avoided interval merge
- added an early empty-`RaySegments` return when a finite cull-safe operand's
  AABB cannot be hit and the ray origin is outside the AABB; the fast panel
  stayed exact, but `drums` measured `11.67`, `11.66`, `11.69` seconds, neutral
  to slightly worse than the Phase-8 mean
- added conservative Roth interval short-circuiting for operands classified as
  always-outside; the fast panel stayed exact, but `drums` regressed to
  `11.79`, `11.83`, `11.80` seconds
- removed a temporary `RaySegmentCrossing` copy inside `mergeByMembership`;
  this was semantically neutral but measured `11.95`, `11.79`, `11.89` seconds
  on `drums`, so it was not kept as a Phase-10 win

Decision:
do not keep these Phase-10 experiments. For `drums`, most hot CSG nodes are
intersections of quadrics and clipping planes; replacing Roth intervals with
Morgan containment is not automatically faster because candidate containment is
also hot. The next viable Phase-10 attempt should be more structural: reduce
`RaySegments` allocation/merge representation itself, or specialize the common
quadric-cylinder-plus-plane intersection pattern rather than switching all
intersections to Morgan filtering.

### Phase 11: Shadow first-blocker CSG traversal

Target:
`drums` spends most time in direct lighting and shadow traversal. The current
branch performs about `6.5M` shadow-ray object tests and then enters the full
baked CSG/all-crossings stack.

Plan:

- add a CSG shadow query that can stop at the first valid blocker when filtered
  shadows are disabled
- for filtered shadows, keep the current all-crossings behavior unless a safe
  ordered-candidate stream can be produced without material loss
- carry light-segment bounds into operand production, not as a late discard
  after all CSG work has already been paid

Risks:

- transparent/filtering shadows require full material processing
- `no_shadow` and child-level attributes must remain exactly equivalent
- early blocker logic is only safe when candidate order is proven by depth

Exit criterion:
`DirectLightShader` and CSG traversal time should fall on `drums` without
changing `kscope`, `pencil`, or the known full-suite AE pattern.

Rejected experiment on 2026-07-01:

- tried a non-filtered-shadow first-blocker path in `DirectLightShader` using
  `BakedTracingCommon::traceObjectFirstHit` before falling back to the existing
  all-crossings scan for ambiguous near/self/no-shadow hits
- standard filtered-shadow panel stayed exact, but the targeted non-filtered
  `drums` run regressed to `12.54`, `12.54`, `12.62` seconds
- the same non-filtered scene without the first-hit attempt measured `11.85`,
  `11.71`, `11.74` seconds
- primitive test counts also increased in the attempted path because baked CSG
  `traceFirstHit` still builds all crossings internally for CSG objects, so the
  first-hit probe added work instead of avoiding it

Decision:
do not keep this Phase-11 first-blocker attempt. A useful Phase-11
implementation needs a real CSG-specific shadow query that produces ordered
candidates or proves a blocker without first building the full CSG
all-crossings queue.

Accepted follow-up on 2026-07-01:

- added `BakedCsgTracing::traceFirstHit` as a real CSG first-hit path
- for `INTERSECTION` CSG, it scans operand crossings and applies membership
  containment directly, keeping only the nearest accepted candidate instead of
  building `RaySegments`
- `BakedSimpleBodyTracing::traceFirstHit` now uses that CSG first-hit path for
  baked CSG bodies; non-intersection CSG still falls back to all-crossings
- the earlier global shadow probe was not kept because `drums` has no shadow
  blockers and the probe duplicated work
- `drums` at `320x200` improved from the Phase-8/10 stable range around
  `11.6s` to `11.37`, `11.44`, `11.35` seconds in standard mode
- non-filtered `drums` with `-qflagsF` measured `11.27`, `11.28`, `11.25`
  seconds, better than the previous non-filtered baseline

Decision:
keep the real baked CSG first-hit path. Phase 11 is complete for this first
safe CSG-first-hit pass, but a true ordered first-blocker shadow query remains
separate future work if filtered shadows need similar treatment.

Accepted follow-up on 2026-07-01 after adding real CSG first-hit:

- retried the non-filtered shadow first-blocker path, but only for compiled
  entries that are direct baked simple bodies with a baked CSG index
- composites and non-CSG bodies stay on the existing all-crossings shadow path,
  avoiding the previous duplicate-work regression
- `drums -qflagsF` at `320x200` measured `11.18`, `11.15`, `11.17` seconds
  with unchanged primitive-test counts, better than the previous `11.27`,
  `11.28`, `11.25` after CSG first-hit and clearly better than the rejected
  global probe
- standard filtered-shadow `drums` measured `11.21`, `11.25`, `11.26` seconds;
  this improvement comes from the CSG first-hit path used by ordinary first-hit
  tracing, not from the non-filtered shadow shortcut

Decision:
keep the restricted non-filtered shadow first-blocker for direct baked CSG
simple bodies. A broader composite-level first-blocker remains blocked until it
can avoid all-crossings without duplicating work on scenes with no blockers.

Full gate on 2026-07-01 after the accepted retry:

- command:
  `./scripts/clean.sh;./scripts/compile.sh;./scripts/renderAll.sh;./scripts/testAgainstGoldenImages.sh`
- compile and full render completed successfully in `187s`
- `testAgainstGoldenImages.sh` exited with code `1` because it requires exact
  image equality, but the AE profile stayed within the operational threshold
  requested for this phase
- observed differences:
  `level1/ballbox1 AE=8`, `level1/texture3 AE=327`,
  `level2/illum2 AE=21`, `level2/pawns AE=5`, `level3/car AE=6`,
  `level3/ionic5 AE=145114`, `math/folium AE=10`, `math/helix AE=92`,
  `math/monkey AE=21`, `math/quarpara AE=133`, `math/tcubic AE=48`,
  `math/trough AE=155`, `math/witch AE=175`
- all differences except the already-expected `level3/ionic5` are far below
  `AE=100000`; the extra math-scene deltas are small enough to treat as
  numerical/noise-level drift for this pass

### Phase 12: Reconsider primitive/direct-write APIs after CSG scratch is stable

Target:
There are still possible wins from direct-writing primitive candidates into
destination queues and avoiding reannotation scans, but doing this safely
requires better ownership of scratch buffers and fresh-candidate ranges.

Plan:

- add an explicit queue mark/range API or a destination wrapper that can expose
  only candidates appended by one primitive call
- replace heuristic scans such as “detail owner count is zero” with structural
  knowledge of which candidates are fresh
- only then widen direct-to-destination paths for transformed operands or
  nested baked CSG

Risks:

- `PriorityQueue` is a heap; appended storage order is not semantic order
- any range API must survive heap sift operations
- active-limit/top-k behavior must be redesigned rather than reused from the
  current `activeLimit`

Exit criterion:
direct-write optimizations become mechanical and auditable rather than
heuristic, and they produce measurable wins on `drums` without image drift.

Phase-12 first pass on 2026-07-01:

- tried a local direct-write fast path for non-transformed `Box` and `Quadric`
  CSG operands, producing already-owned candidates directly in
  `BakedCsgTracing` instead of letting `Geometry` emit ownerless hits followed
  by `annotateDirectCandidates`
- the fast golden panel stayed exact on `kscope` and `pencil`, but performance
  regressed: `level2/iortest` moved to `5.71s`, `level3/piece3` to `2.30s`,
  and `drums` to `13.00s`
- `drums` primitive counts showed `Quadric` tests rising from the stable
  `41178805` range to `65963178`, so the shortcut changed traversal behavior
  enough to be unsafe as a local render-layer specialization

Decision:
do not keep primitive-type `dynamic_cast` direct-write in `BakedCsgTracing`.
If primitive direct-write is retried, it should be expressed as an explicit
`Geometry`-level API so each primitive owns candidate production exactly once
and the CSG layer does not duplicate primitive dispatch semantics.

Phase-12 second pass on 2026-07-01:

- tried using a scratch queue as an explicit fresh-candidate range only when
  the destination queue already had candidates
- the idea was semantically conservative: primitive writes into an empty
  scratch queue, the scratch queue is annotated as fresh ownership, then the
  candidates are transferred into the destination heap
- the fast panel stayed in the good range (`iortest 4.39s`, `piece3 2.13s`,
  `kscope AE=0`, `pencil AE=0`), but `drums` measured `11.40`, `11.37`,
  `11.37` seconds, not a measurable win over the Phase-11 stable range

Decision:
do not keep the scratch-range variant either. It proves the ownership model is
auditable, but the extra queue traffic cancels the scan savings on the target
scene.

Follow-up direction:
do not pursue a CSG-owned linear stream as the primary route. It may reduce
heap conversions, but it would also replace a large part of the current
`RaySegments` semantics with a new interval/event pipeline. That is too much
implementation complexity for the current stabilization goal. The next work
should instead keep `RaySegments` intact and introduce an explicit
`Geometry`-level writer API for already-annotated candidate emission, avoiding
both heap-range inference and scratch transfer while preserving the current CSG
merge model.

### Phase 13: Geometry annotated-candidate writer API

Target:
Move ownership/material annotation into an explicit primitive emission API so
CSG does not need to infer fresh candidates from heap state.

Plan:

- add a small writer/context type that carries:
  candidate destination queue, material override, detail owner, and
  `materialUsesObjectLocalPoint` policy
- add a virtual `Geometry` method for all-crossings emission with attributes
  already applied by the primitive, leaving the existing
  `doIntersectionForAllRayCrossings` API as the compatibility path
- default the new virtual method to call the existing method plus a local,
  owned annotation step, so non-migrated geometry remains correct
- do not change `Scene`, `BakedScene`, or `RaySegments` in this phase
- first implement the API shape; activation in baked CSG is allowed only if it
  does not regress the hot scenes

Risks:

- the default fallback must not annotate old candidates from an already-used
  heap
- transformed operands still need point/t re-projection in `BakedCsgTracing`;
  the primitive writer must not hide that responsibility
- `no_shadow`, texture local point, and detail-owner stacking must remain
  byte-for-byte equivalent in the accepted scenes

Exit criterion:
the new API compiles across the geometry hierarchy, default fallback is exact,
and no hot path is switched to the fallback if it regresses `drums` or the
fixed panel.

Phase-13 result on 2026-07-01:

- added `GeometryIntersectionEmissionContext` with material override, detail
  owner, and `materialUsesObjectLocalPoint` policy
- added virtual `Geometry::doIntersectionForAllRayCrossingsAnnotated`
- default implementation keeps legacy geometry behavior, but annotates only
  candidates owned by the call: if the destination queue is empty, it annotates
  that queue directly; otherwise it uses a scratch queue before transferring
  candidates into the destination heap
- no `Scene`, baked-scene, or `RaySegments` structure changed
- a narrow baked CSG activation was tested and rejected: visual checks stayed
  exact, but the fallback path regressed `drums` to `11.63s` and then `11.58s`
  after limiting it to empty destination queues
- after removing the active CSG routing and keeping only the API foundation,
  the panel measured `spline 0.30s`, `ntreal 0.66s`, `piece3 2.12s`,
  `iortest 4.46s`, `shapes2 0.13s`, with `kscope AE=0` and `pencil AE=0`;
  `drums` measured `11.51s`

Decision:
keep the `Geometry` annotated-emission API foundation, but do not activate it
from baked CSG through the default fallback. Phase 14 must add native primitive
overrides first, starting with `Quadric`, and only then should Phase 15 switch
the baked CSG hot path to the writer API.

### Phase 14: Migrate hot primitives to native annotated emission

Target:
Remove fallback annotation work for the primitives that dominate the current
hot paths, without duplicating primitive dispatch in `BakedCsgTracing`.

Plan:

- migrate `Quadric` first because `drums` is quadric-heavy
- migrate `Box` and `Sphere` next only after `Quadric` is stable
- keep each primitive's existing intersection helper as the single source of
  truth; the new method should only change candidate construction/annotation
- measure each primitive migration separately on:
  `drums`, `level2/iortest`, and the fixed five-scene panel
- reject a primitive migration if it increases primitive test counts or changes
  golden-image AE beyond the accepted noise pattern

Risks:

- rebuilding candidate creation inside each primitive can accidentally diverge
  from current material/owner behavior
- `Sphere` has a private intersection helper today; exposing or reusing it must
  be done without changing its affine-depth assumptions
- direct primitive wins may be small if the remaining cost is dominated by
  containment tests or `RaySegments` merges rather than annotation

Exit criterion:
at least one hot primitive uses native annotated emission and produces a
measurable win on `drums` or `iortest` with no golden-image regression.

Phase-14 result on 2026-07-01:

- added `Geometry::hasNativeAnnotatedCrossings()` so baked CSG can call the
  annotated API only when a primitive has a native override; the generic
  fallback remains available but is not used in the hot CSG path
- migrated `Quadric` first; `drums` measured `11.19`, `11.23`, `11.22`
  seconds with unchanged primitive-test counts
- migrated `Box`; the fast panel remained exact and `drums` stayed acceptable
  at `11.32s`
- migrated `Sphere` without exposing its private intersection helper; the fast
  panel measured `spline 0.30s`, `ntreal 0.66s`, `piece3 2.13s`,
  `iortest 4.36s`, `shapes2 0.12s`, with `kscope AE=0` and `pencil AE=0`
- final `drums` samples with `Quadric`, `Box`, and `Sphere` native emission
  active were `11.21`, `11.31`, `11.19` seconds, with primitive counts
  unchanged

Full gate on 2026-07-01:

- command:
  `./scripts/clean.sh;./scripts/compile.sh;./scripts/renderAll.sh;./scripts/testAgainstGoldenImages.sh`
- compile and full render completed successfully in `185s`
- `testAgainstGoldenImages.sh` exited with code `1` because it requires exact
  image equality, but the AE profile matched the accepted Phase-11 pattern:
  `level1/ballbox1 AE=8`, `level1/texture3 AE=327`,
  `level2/illum2 AE=21`, `level2/pawns AE=5`, `level3/car AE=6`,
  `level3/ionic5 AE=145114`, `math/folium AE=10`, `math/helix AE=92`,
  `math/monkey AE=21`, `math/quarpara AE=133`, `math/tcubic AE=48`,
  `math/trough AE=155`, `math/witch AE=175`
- all differences except the already-expected `level3/ionic5` stay well below
  `AE=100000`

Decision:
keep the native annotated-emission overrides for `Quadric`, `Box`, and
`Sphere`, and keep baked CSG activation gated by `hasNativeAnnotatedCrossings`.
This closes Phase 14 and effectively completes the safe direct-operand
activation intended for Phase 15. Any broader writer activation for transformed
operands or nested CSG remains future work and should require a separate phase.

### Phase 15: Activate Geometry writer in baked CSG only

Target:
Use the new annotated-emission API in `BakedCsgTracing` where it replaces the
current heuristic `annotateDirectCandidates` path, while leaving legacy
geometry traversal unchanged.

Plan:

- switch only no-transform, non-nested, non-plane baked CSG operands to the new
  geometry writer
- keep transformed operands, nested CSG, and special plane handling on the
  existing paths until separate measurements prove a safe win
- remove or narrow `annotateDirectCandidates` only after all direct primitive
  users have moved to the writer API
- run the full mandatory gate before marking the phase complete

Risks:

- widening this too early can repeat the rejected Phase-12 render-layer
  direct-write regression
- the writer must integrate with heap ordering only by `offer`, not by assuming
  append/range order
- CSG containment-sensitive scenes remain the main correctness risk

Exit criterion:
the writer-backed baked CSG path improves or holds `drums`, keeps the five-scene
panel in the current good range, and passes the full image gate under the
accepted AE threshold.

Phase-15 note:
The safe subset of this phase was completed as part of Phase 14: baked CSG now
routes no-transform, non-nested, non-plane operands through the writer only
when the concrete geometry reports native annotated emission support. Do not
widen to fallback-based writer activation.

### Phase 16: Low-risk remaining primitive writer coverage

Target:
Increase native annotated-emission coverage for simple direct primitives that
can reuse their existing intersection helper without changing transforms,
`RaySegments`, or CSG containment logic.

Scope:

- migrate `Triangle`, because it emits a single candidate and has a compact
  existing intersection path
- do not migrate `PolynomialShape`, `Blob`, `HeightField`, or
  `ParametricBiCubicPatch` in this phase; those paths carry additional solver,
  transform, or internal scratch semantics and need separate profiling
- do not widen to transformed or nested baked CSG operands

Result on 2026-07-01:

- added native annotated emission support to `Triangle`
- panel measured `spline 0.30s`, `ntreal 0.67s`, `piece3 2.14s`,
  `iortest 4.39s`, `shapes2 0.12s`
- golden short checks remained exact: `kscope AE=0`, `pencil AE=0`
- `drums` measured `11.24s` with unchanged primitive-test counts

Decision:
keep `Triangle` native annotated emission. Phase 16 is complete for the
low-risk primitive coverage pass. The remaining primitive families are not
"future work" outside this plan; they are split into explicit follow-up phases
because each one has different solver/scratch/transform semantics and needs its
own acceptance gate.

### Phase 17: PolynomialShape annotated emission

Target:
Add native annotated emission for `PolynomialShape` while preserving its
solver behavior and object/world point handling.

Plan:

- keep `PolynomialShape::intersect` and `intersectQuartic` as the single source
  of root generation
- add `doIntersectionForAllRayCrossingsAnnotated` that mirrors the existing
  candidate creation and applies the `GeometryIntersectionEmissionContext`
  directly
- do not change polynomial transform logic, root de-duplication, or hit-depth
  ordering in this phase
- measure `math/*` scenes explicitly, because polynomial objects are heavily
  represented there and the accepted full-gate drift already includes several
  small math-scene AEs

Required checks:

- `./scripts/compile.sh`
- `./scripts/benchmarkPanel.sh --with-goldens`
- render/compare targeted math scenes that exercise polynomial shapes
- full gate before marking complete, because `PolynomialShape` affects the
  current accepted AE pattern in math scenes

Exit criterion:
native annotated emission for `PolynomialShape` is active, panel timings stay
in range, and the full-gate AE profile does not exceed the accepted threshold.

Phase-17 result on 2026-07-01:

- added native annotated emission support to `PolynomialShape`
- kept `PolynomialShape::intersect` and `intersectQuartic` as the only root
  generation paths; the annotated method mirrors legacy candidate creation,
  including transform handling, root de-duplication, and depth calculation
- panel measured `spline 0.34s`, `ntreal 0.67s`, `piece3 2.17s`,
  `iortest 4.45s`, `shapes2 0.12s`; golden short checks remained exact:
  `kscope AE=0`, `pencil AE=0`
- targeted math scenes matched the accepted AE pattern:
  `folium AE=10`, `helix AE=92`, `monkey AE=21`, `quarpara AE=133`,
  `tcubic AE=48`, `trough AE=155`, `witch AE=175`
- full gate command:
  `./scripts/clean.sh;./scripts/compile.sh;./scripts/renderAll.sh;./scripts/testAgainstGoldenImages.sh`
- full render completed in `185s`; the comparison script exited `1` because it
  requires exact equality, but the AE profile matched the accepted pattern:
  `level1/ballbox1 AE=8`, `level1/texture3 AE=327`,
  `level2/illum2 AE=21`, `level2/pawns AE=5`, `level3/car AE=6`,
  `level3/ionic5 AE=145114`, `math/folium AE=10`, `math/helix AE=92`,
  `math/monkey AE=21`, `math/quarpara AE=133`, `math/tcubic AE=48`,
  `math/trough AE=155`, `math/witch AE=175`

Decision:
keep `PolynomialShape` native annotated emission. Phase 17 is complete.

### Phase 18: Blob annotated emission

Target:
Add native annotated emission for `Blob` without changing blob solver state,
component handling, or normal/detail-owner behavior.

Plan:

- inspect `Blob::doIntersectionForAllRayCrossings` for internal temporary state
  and candidate ownership assumptions before editing
- implement the annotated variant by reusing the same hit-generation code path
  or by factoring candidate finalization into a shared helper
- avoid duplicating solver logic if doing so would risk divergence between the
  legacy and annotated paths

Required checks:

- `./scripts/compile.sh`
- `./scripts/benchmarkPanel.sh --with-goldens`
- targeted scenes containing blobs, selected from the render suite
- full gate if any targeted image differs from the current accepted pattern

Exit criterion:
`Blob` has native annotated emission, targeted blob scenes remain visually
stable, and no panel or `drums` regression appears.

Phase-18 result on 2026-07-01:

- added native annotated emission support to `Blob`
- factored the legacy intersection path through `Blob::traceCrossings`, so the
  annotated and non-annotated paths share the same solver, interval handling,
  component coefficient scratch, validation, transform, and normal ownership
  assumptions
- panel measured `spline 0.30s`, `ntreal 0.66s`, `piece3 2.14s`,
  `iortest 4.35s`, `shapes2 0.11s`; golden short checks remained exact:
  `kscope AE=0`, `pencil AE=0`
- targeted blob scene `level1/blob` rendered at `1280x800` in `0.86s` and
  compared exact: `AE=0`, `RMSE=0`
- full gate command:
  `./scripts/clean.sh;./scripts/compile.sh;./scripts/renderAll.sh;./scripts/testAgainstGoldenImages.sh`
- full render completed in `185s`; the comparison script exited `1` because it
  requires exact equality, but the AE profile matched the accepted pattern:
  `level1/ballbox1 AE=8`, `level1/texture3 AE=327`,
  `level2/illum2 AE=21`, `level2/pawns AE=5`, `level3/car AE=6`,
  `level3/ionic5 AE=145114`, `math/folium AE=10`, `math/helix AE=92`,
  `math/monkey AE=21`, `math/quarpara AE=133`, `math/tcubic AE=48`,
  `math/trough AE=155`, `math/witch AE=175`

Decision:
keep `Blob` native annotated emission. Phase 18 is complete.

### Phase 19: HeightField annotated emission

Target:
Evaluate and, only if safe, add native annotated emission for `HeightField`.

Plan:

- profile whether `HeightField` participates in baked CSG direct operands often
  enough to justify migration
- inspect its internal queue usage and multi-hit emission carefully; it has more
  scratch/state than the simple primitives migrated so far
- if migration is safe, add an annotated path that shares candidate finalization
  with the legacy path instead of duplicating traversal logic

Required checks:

- `./scripts/compile.sh`
- `./scripts/benchmarkPanel.sh --with-goldens`
- targeted height-field scenes from the suite
- full gate if activated in any broad CSG path

Exit criterion:
either `HeightField` native annotated emission is implemented and validated, or
the phase documents a measured reason to leave it on the legacy path.

Phase-19 result on 2026-07-01:

- added native annotated emission support to `HeightField`
- factored the legacy intersection path through `HeightField::traceCrossings`,
  so annotated and non-annotated traversal share the same local ray transform,
  bounding-box test, traversal state, multi-hit emission, and block/pixel
  traversal logic
- applied detail-owner annotation once to the reused
  `IntersectionCandidate` before traversal, avoiding repeated owner pushes for
  multi-hit emission
- panel measured `spline 0.31s`, `ntreal 0.65s`, `piece3 2.15s`,
  `iortest 4.38s`, `shapes2 0.12s`; golden short checks remained exact:
  `kscope AE=0`, `pencil AE=0`
- targeted height-field scenes were exact:
  `level2/hfclip AE=0`, `level2/sunsethf AE=0`, `level2/mtmand AE=0`
- full gate command:
  `./scripts/clean.sh;./scripts/compile.sh;./scripts/renderAll.sh;./scripts/testAgainstGoldenImages.sh`
- full render completed in `184s`; the comparison script exited `1` because it
  requires exact equality, but the AE profile matched the accepted pattern:
  `level1/ballbox1 AE=8`, `level1/texture3 AE=327`,
  `level2/illum2 AE=21`, `level2/pawns AE=5`, `level3/car AE=6`,
  `level3/ionic5 AE=145114`, `math/folium AE=10`, `math/helix AE=92`,
  `math/monkey AE=21`, `math/quarpara AE=133`, `math/tcubic AE=48`,
  `math/trough AE=155`, `math/witch AE=175`

Decision:
keep `HeightField` native annotated emission. Phase 19 is complete.

### Phase 20: Parametric patch annotated emission

Target:
Evaluate annotated emission for `ParametricBiCubicPatch` / parametric patches
without disturbing their per-ray scratch and post-intersection data flow.

Plan:

- review the patch intersection scratch object and any required extra data used
  later by normal/shading
- only add native annotated emission if candidate annotation can be separated
  from patch-specific scratch lifetime
- keep patch solver and ordering unchanged

Required checks:

- `./scripts/compile.sh`
- `./scripts/benchmarkPanel.sh --with-goldens`
- targeted patch scenes, especially those covering normals/reflection if
  present in the suite
- full gate if the patch writer becomes active in baked CSG

Exit criterion:
parametric patch native annotated emission is either implemented with stable
targeted images or explicitly rejected with measurements and risk notes.

Phase-20 result on 2026-07-01:

- added native annotated emission support to `ParametricBiCubicPatch`
- kept `ParametricBiCubicSolver::allParametricBiCubicPatchIntersections` as
  the only solver/scratch writer; the patch wrapper now applies material and
  detail-owner attributes to the candidates produced by the solver
- preserved the thread-local patch scratch used later by normal/shading; no
  solver ordering or intersection scratch lifetime changed
- panel measured `spline 0.31s`, `ntreal 0.66s`, `piece3 2.19s`,
  `iortest 4.49s`, `shapes2 0.12s`; golden short checks remained exact:
  `kscope AE=0`, `pencil AE=0`
- targeted patch scenes were exact:
  `math/bezier AE=0`, `math/bezier0 AE=0`, `level3/teapot AE=0`
- full gate command:
  `./scripts/clean.sh;./scripts/compile.sh;./scripts/renderAll.sh;./scripts/testAgainstGoldenImages.sh`
- full render completed in `186s`; the comparison script exited `1` because it
  requires exact equality, but the AE profile matched the accepted pattern:
  `level1/ballbox1 AE=8`, `level1/texture3 AE=327`,
  `level2/illum2 AE=21`, `level2/pawns AE=5`, `level3/car AE=6`,
  `level3/ionic5 AE=145114`, `math/folium AE=10`, `math/helix AE=92`,
  `math/monkey AE=21`, `math/quarpara AE=133`, `math/tcubic AE=48`,
  `math/trough AE=155`, `math/witch AE=175`

Decision:
keep `ParametricBiCubicPatch` native annotated emission. Phase 20 is complete.

### Phase 21: Make `SingleCorePlaneIntersection` use its core operand

Target:
`BakedCsgTracing::traceSingleCorePlaneIntersection` currently receives a
`coreIndex` selected during scene compilation, but the traversal still scans
every operand generically and discards the index. This leaves an accepted
specialization behaving mostly like the generic Morgan intersection path.

Plan:

- validate the baked `coreIndex` before using the specialized path
- trace the single non-plane core operand explicitly
- keep plane operand candidates, because clipping planes can still be visible
  CSG boundaries
- use core-first membership checks for plane candidates and plane-only checks
  for core candidates
- preserve candidate emission order unless a benchmark proves the order change
  is exact across the full image gate

Risks:

- plane candidates can be real visible caps; removing them is not safe
- changing containment-test order can expose tolerance-sensitive one-pixel
  differences
- any fallback for malformed baked data must preserve the generic Morgan
  behavior

Exit criterion:
`drums` should show a measurable reduction in the single-core plane
intersection path, with no new broad image differences.

Phase-21 first pass:

- `traceSingleCorePlaneIntersection` now validates and uses the baked
  `coreIndex`
- the traversal keeps plane candidates, but routes membership checks through a
  core-first helper for plane candidates and a plane-only helper for core
  candidates
- `drums` at `320x200` measured `11.11`, `11.16`, `11.08` seconds
- comparison against the previous current `drums` image stayed exact:
  `AE=0`
- comparison against the older baked baseline stayed at the known small delta:
  `AE=71`

Decision:
keep the change. It is a small but safe improvement and makes the specialization
match its baked classification data.

### Phase 22: Reduce local-ray clone pressure

Target:
`drums` still reports very high `RayWithSegments::LocalIntersectionClone`
traffic in baked CSG traversal.

Plan:

- introduce a lighter local-ray representation or resettable scratch ray for
  operand-space transforms
- avoid copying material/detail containers when a transformed operand only
  needs origin, direction, and cached quadric state
- keep the current full clone for paths that really need segment/material
  container inheritance

Risks:

- refraction and material-stack behavior depend on the full `RayWithSegments`
  state in some paths
- a lighter ray must not silently drop data required by nested CSG or shading

Exit criterion:
local-ray clone counts fall materially in `gprof`, and the fixed panel plus
`drums` keep the accepted image profile.

Rejected first pass:

- tried a reusable local-ray scratch stack inside `BakedCsgTracing` so
  transformed operands could reset `RayWithSegments` instances instead of
  constructing `LocalIntersectionClone` on the stack for every operand
- added a reset method that copied origin, direction, `t`, flags, statistics,
  configuration, and queue-pool ownership while leaving containing-media stacks
  empty, matching the existing intersection-only clone semantics
- the change compiled and kept the known `drums` baseline delta at `AE=71`
- performance regressed: `drums` at `320x200` measured `12.08`, `12.15`,
  `12.11` seconds, worse than the Phase-21 mean of about `11.12s`

Decision:
do not keep the reusable local-ray scratch stack. The heap-allocated scratch
objects and borrow/return bookkeeping cost more than the already-cheap stack
clone. A future Phase 22 attempt should avoid both allocation and extra
bookkeeping, or target a narrower clone site proven hot by `gprof`.

### Phase 23: Share CSG scratch queues from baked simple-body traversal

Target:
`BakedSimpleBodyTracing` still borrows temporary queues directly from
`PriorityQueuePool` before entering baked CSG. That bypasses the reusable
`CsgScratchContext` used inside `BakedCsgTracing`.

Plan:

- pass a scratch context from the outer baked simple-body crossing path into
  CSG traversal
- reuse the same scratch queue stack for object, geometry, and nested CSG
  crossing work
- keep queue clearing explicit at every borrow/return boundary

Risks:

- stale candidates in a reused queue can create hard-to-debug CSG artifacts
- recursive CSG needs strict stack discipline or overflow fallback behavior

Exit criterion:
queue-pool traffic falls again in `gprof` without changing candidate ordering
or image stability.

Rejected first pass:

- exposed the baked CSG scratch context so `BakedSimpleBodyTracing` could borrow
  its local CSG result queue from the same reusable queue stack used inside
  `BakedCsgTracing`
- also routed baked CSG first-hit calls through the shared context
- the change compiled and preserved image stability on `drums`: `AE=0` against
  the previous current image and the known `AE=71` against the older baked
  baseline
- performance regressed: `drums` at `320x200` measured `11.74`, `11.76`,
  `11.75` seconds, worse than the Phase-21 mean of about `11.12s`

Decision:
do not keep outer simple-body ownership of the CSG scratch context. The extra
API boundary and context-management work outweighed any saved queue-pool
traffic for `drums`. Retry Phase 23 only if a future profile shows a specific
outer `pop/push` site dominating again.

### Phase 24: Add a transformed simple-operand fast path

Target:
Transformed primitive CSG operands still pay for local scratch queues and
candidate transfer even when they are not nested CSG nodes.

Plan:

- start with transformed simple primitives that already have native annotated
  emission
- intersect in operand-local space, transform candidates back, and emit into
  the destination queue directly when ownership and point-space semantics are
  proven equivalent
- keep nested CSG and transformed planes on their current specialized paths
  until they have separate correctness proof

Risks:

- material coordinate space and `doExtraInformation` owner stacks are fragile
- previous transformed operand pre-baking experiments caused full-suite image
  failures, so this must be narrower and easier to audit

Exit criterion:
the path reduces scratch queue transfer in `drums` while keeping `cantelop`,
`pacman`, `kscope`, `pencil`, and the full gate inside the accepted AE profile.

Rejected first pass:

- tried routing transformed, non-nested, non-plane operands through native
  annotated emission when `hasNativeAnnotatedCrossings()` was true
- the local scratch queue was still required so candidate points and `t` could
  be transformed back to the parent ray space, but the post-transfer
  owner/material-space annotation pass could be skipped
- first implementation measured `drums` at `11.18`, `11.28`, `11.23` seconds
- after removing a per-candidate virtual `hasNativeAnnotatedCrossings()` check,
  the variant measured `11.15`, `11.19`, `11.18` seconds
- both variants were exact against the previous current `drums` image
  (`AE=0`) and preserved the known `AE=71` delta against the older baked
  baseline

Decision:
do not keep this transformed native-annotation path. It is visually safe, but
the timing remains neutral relative to Phase 21 and does not reduce the core
scratch queue / candidate-transfer cost. A future Phase 24 attempt must avoid
the local scratch queue entirely or target a more specific transformed operand
pattern.

### Phase 25: Resolve the `drums` baseline image delta

Target:
The older baked baseline and the current scene-owned baked layer differ by
`AE=71` at `320x200`. That is small, but it weakens performance comparisons
because it means the two paths are not strictly identical.

Plan:

- localize the differing pixels in `drums`
- identify whether they come from CSG containment tolerance, candidate ordering,
  normal/detail reconstruction, or material point-space handling
- fix only if the root cause is a real semantic drift; otherwise document why
  the older baked branch is not an exact target for those pixels

Risks:

- chasing tiny numerical differences can consume time without improving the
  performance target
- changing tolerances globally can destabilize much larger parts of the suite

Exit criterion:
either `drums` becomes exact against the older baked baseline, or the remaining
delta has a documented semantic explanation and does not mask timing work.

Phase-25 result:

- generated a direct pixel comparison between
  `4af1a75e5b7356600bec34e12a4882560994a058` and the current Phase-21 code
  using `drums` at `320x200`
- the delta remains exactly the known `AE=71`, with
  `RMSE=344.851 (0.00526208)`
- the 71 differing pixels are sparse and non-contiguous; their bounding box is
  `(26,35)-(294,178)`, and `70` of `71` have a mirrored counterpart around the
  scene center
- the differences are not just channel-rounding noise:
  `10` pixels have Manhattan RGB delta `<=3`, `20` are in `4..30`, and `41`
  are larger than `30`
- render statistics also differ in the same narrow area of behavior:
  baseline reports `141879` total rays and `46899` reflected rays, while the
  current code reports `141821` total rays and `46841` reflected rays; the
  transmitted-ray count stays identical at `30980`
- visual inspection of an enlarged diff overlay shows isolated points on drum
  silhouettes, small highlights, and CSG/edge details rather than a coherent
  lighting or texture region

Interpretation:
the older baked branch and the scene-owned baked branch are not byte-identical
on a tiny set of CSG/edge pixels. The current evidence points to hit
classification / candidate ordering at reflective boundary pixels: a different
surface choice on those pixels changes whether a reflection ray is spawned,
which explains the exact `58`-ray drop in both total rays and reflected rays.
This is not a global material, color, gamma, or image-output difference.

Decision:
do not change tolerances or CSG selection logic just to force `AE=0` against
the older baked branch. The remaining `AE=71` is documented as a narrow
semantic drift between two baked implementations and should not block
performance work unless a later change increases the delta or turns it into a
structured heatmap.

Practical sequencing note:
because the current worktree already contains the simplest scene-entry broad
phase that measured well, the next iteration should not spend more time on
top-level traversal structures. The next useful branch should begin with
Phase 13. Phase 12 showed that render-layer direct-write is the wrong
abstraction boundary; the lower-risk path is to make primitive annotated
emission explicit in `Geometry` and activate it narrowly from baked CSG.

## Current Mid-Plan State And Handoff Notes

This section is intentionally explicit because recent attempts made incorrect
changes that were plausible locally but wrong globally.

### Phase status table

| Phase | Current state | What remains |
| --- | --- | --- |
| Phase 0: fixed benchmark harness | Active and mandatory | Keep measuring the same five scenes against `4af1a75e5b7356600bec34e12a4882560994a058`. |
| Phase 1: scene compilation | Complete as infrastructure | Keep legacy algorithms only as fallbacks for not-yet-baked node kinds, mainly CSG. |
| Phase 2: flatten simple objects | Complete for current baked graph scope | Flat `SimpleBody`, `Composite` containers, helper subgraphs, and CSG graph records are baked. Remaining cleanup is semantic-owner reduction, not missing flattening infrastructure. |
| Phase 3: broad phase | Complete for current safe scope | Top-level AABB culling is active for primary and shadow rays; composite child culling is active only where ray/bounds space is proven safe and order is preserved. |
| Phase 4: dedicated first-hit path | Complete for the common safe path | Dedicated no-queue first-hit is active for baked simple bodies whose primitive supports it and that have no CSG, bounding helpers, or clipping helpers. Keep the all-crossings path for complex cases. |
| Phase 5: baked CSG | Complete for current baked graph scope | CSG operand graphs, transforms, bounds, nested references, safe fast paths, and containment specializations are baked and active. Future CSG speed work belongs to Phase 6/post-bake optimization. |
| Phase 6: remaining scaffolding | Complete for current safe pass | Avoided unused composite-local ray clones. Queue active-limit and direct-write candidate shortcuts remain intentionally blocked unless their heap semantics are redesigned. |
| Phase 7: drums CSG-heavy rebaseline | Complete as analysis | Keep `drums` as an active regression scene until the current scene-owned baked path approaches the older baked branch. |
| Phase 8: CSG queue churn | Complete for first scratch-context pass | Queue-pool calls dropped materially on `drums`, but wall-clock gain was small; continue with Phase 9/10 for local-ray clone and `RaySegments` cost. |
| Phase 9: pretransform simple CSG operands | Pending after rejected experiment | The plane/quadric pretransform variant failed the full golden-image gate and was not kept; only retry with a safer transformed-operand strategy. |
| Phase 10: RaySegments specialization | Pending after rejected experiments | Morgan-routing, AABB empty-segment, always-outside short-circuit, and merge-copy cleanup attempts did not improve `drums`; retry with a deeper interval representation change or a scene-pattern specialization. |
| Phase 11: shadow first-blocker CSG | Complete for first CSG-first-hit pass | Added real baked CSG `traceFirstHit`; restricted non-filtered shadow first-blocker now uses it for direct baked CSG simple bodies. Composite-wide blocker queries remain future work. |
| Phase 12: direct-write/range API | Complete as rejected design pass | Render-layer primitive direct-write and scratch fresh-range variants were measured and rejected. Do not pursue the CSG-owned linear stream route for now; prefer explicit `Geometry` annotated emission. |
| Phase 13: Geometry annotated-candidate writer API | Complete as API foundation | Added `GeometryIntersectionEmissionContext` and fallback annotated emission without changing `Scene` or `RaySegments`; active baked CSG routing through the fallback was measured and rejected. |
| Phase 14: hot primitive migration | Complete | `Quadric`, `Box`, and `Sphere` now have native annotated emission; `drums` and panel measurements stayed in range and full gate matched accepted AE pattern. |
| Phase 15: baked CSG writer activation | Complete for native direct operands | Baked CSG routes only no-transform, non-nested, non-plane operands with native annotated support through the writer API. Broader transformed/nested activation is future work. |
| Phase 16: low-risk remaining primitive writer coverage | Complete | Added native annotated emission for `Triangle`; panel and short golden checks stayed in range. |
| Phase 17: PolynomialShape annotated emission | Complete | Added native annotated emission for polynomial shapes; targeted math scenes and full gate matched accepted AE pattern. |
| Phase 18: Blob annotated emission | Complete | Added native annotated emission for `Blob` through a shared `traceCrossings` path; targeted blob scene was exact and full gate matched accepted AE pattern. |
| Phase 19: HeightField annotated emission | Complete | Added native annotated emission for `HeightField` through a shared traversal path; targeted height-field scenes were exact and full gate matched accepted AE pattern. |
| Phase 20: Parametric patch annotated emission | Complete | Added native annotated emission for `ParametricBiCubicPatch`; targeted patch scenes were exact and full gate matched accepted AE pattern. |
| Phase 21: `SingleCorePlaneIntersection` core operand | Complete for first pass | `coreIndex` is now used explicitly; `drums` improved to an `11.12s` mean and stayed exact against the previous current image. |
| Phase 22: local-ray clone pressure | Pending after rejected first pass | Reusable local-ray scratch inside `BakedCsgTracing` regressed `drums`; retry only with a narrower or allocation-free clone reduction. |
| Phase 23: baked simple-body CSG scratch sharing | Pending after rejected first pass | Sharing CSG scratch ownership from `BakedSimpleBodyTracing` regressed `drums`; retry only if a future profile shows the outer queue site dominating again. |
| Phase 24: transformed simple-operand fast path | Pending after rejected first pass | Native annotated emission for transformed simple operands was exact but timing-neutral; retry only with a design that removes the local scratch queue or targets a specific transformed operand pattern. |
| Phase 25: `drums` baseline image delta | Complete as analysis | The persistent `AE=71` is sparse, mirrored, and tied to reflected-ray count changes at CSG/edge pixels; do not chase exactness unless future changes enlarge or structure the delta. |

### Current measured panel

After the nested-composite fix, baked CSG reactivation, the direct primitive
operand fast path, and the inverse-containment bugfix, the `320x200` timing
panel measured:

| Scene | Baseline 4af1a75 (s) | Current worktree (s) | Factor |
| --- | ---: | ---: | ---: |
| `level2/spline` | 1.989 | 0.325 | 0.163 |
| `level3/ntreal/ntreal` | 1.292 | 0.672 | 0.520 |
| `level3/piece3/piece3` | 4.112 | 2.443 | 0.594 |
| `level2/iortest` | 5.316 | 4.216 | 0.793 |
| `level1/shapes2` | 0.382 | 0.132 | 0.346 |

Reading:

- all five panel scenes are now faster than the baseline
- `iortest` is no longer the outstanding regression; the next step should be
  chosen from profiling or code-structure opportunity, not from outdated panel
  assumptions in earlier sections of this document
- image sensitivity still clusters around CSG containment/classification, so
  `cantelop`, `pacman`, `kscope`, and `pencil` remain the required semantic
  guards for any Phase-5 follow-up

### Mandatory image checks before accepting any bake change

The five performance scenes are not enough to guard composite semantics.
Every change touching `Scene`, `BakedSimpleBodyTracing`,
`BakedCompositeTracing`, direct-light shadow traversal, or CSG traversal must
also run these golden checks:

| Scene | Why it matters | Required result |
| --- | --- | --- |
| `level3/kscope` | Nested composites, reflections/refractions, transparent objects, and sensitive shadow/visibility paths. | `AE=0` and `RMSE=0` against `../referenceTestImages/level3/kscope.tga`. |
| `level3/pencil` | Nested composites, bounding regions, image-map texture, and composite transforms. | `AE=0` and `RMSE=0` against `../referenceTestImages/level3/pencil.tga`. |

Known-good command shape:

```bash
./build/povray +letc/include +ietc/level3/kscope.pov \
  +ooutput/level3/kscope.tga +w1280 +h800 -d -v +x +ft

( cd etc/level3/pencil && ../../../build/povray +l../../include +ipencil.pov \
  +o../../../output/level3/pencil.tga +w1280 +h800 -d -v +x +ft )

compare -metric AE output/level3/kscope.tga \
  ../referenceTestImages/level3/kscope.tga null:
compare -metric AE output/level3/pencil.tga \
  ../referenceTestImages/level3/pencil.tga null:
```

Current corrected result on 2026-06-30:

| Scene | AE | RMSE |
| --- | ---: | ---: |
| `level3/kscope` | 0 | 0 |
| `level3/pencil` | 0 | 0 |

### Composite bake invariants

`BakedCompositeTracing` must preserve the observable behavior of
`Composite::doIntersectionForAllRayCrossings`.

Do not break these invariants:

- Child traversal order is `getSimpleBodies().size() - 1` down to `0`.
- Do not split composite children into bounded and unbounded passes unless the
  resulting candidate ordering is proven identical.
- A baked composite parent must reserve its `bakedCompositeIndex` before
  compiling its children. Nested composites can append their own baked records
  during recursive compilation; assigning the parent index after child baking
  can make the parent point to the wrong baked node.
- Composite bounding shapes are evaluated in composite-local ray space before
  child traversal.
- Composite clipping shapes are evaluated after child intersections are found,
  using the same point-space convention as the legacy path.
- Every accepted child candidate must push the previous `hitBody` as a detail
  owner, then set `hitBody` to the composite. This is required for normal
  reconstruction through nested composites and CSG.
- Composite candidate `t` currently follows legacy behavior: it is recomputed
  from the final point using `.length()`, not the signed projection used by
  `SimpleBody`. Do not "fix" this inside the bake path unless the legacy path
  changes too and the golden suite confirms it.
- CSG children inside composites must remain on the legacy CSG path until
  Phase 5 supplies a baked CSG implementation.

### Simple-body bake invariants

`BakedSimpleBodyTracing` is active for primary rays and shadow rays. It must
remain equivalent to `SimpleBody::doIntersectionForAllRayCrossings`.

Do not break these invariants:

- Bounding shapes are checked before geometry traversal.
- Clipping shapes are evaluated after geometry-local hits are transformed back
  to object-local space.
- Object texture, color, `no_shadow`, and `hitBody` attributes must be written
  exactly as the legacy path does.
- Geometry material override behavior must be preserved. CSG and material
  inheritance bugs often surface as broad heatmap changes rather than crashes.
- `traceFirstHit` uses the direct no-queue primitive API only for the safe flat
  simple-body path. CSG, clipping-heavy objects, and bodies with bounding helper
  shapes must continue to use all-crossings until each shortcut has a separate
  exactness proof.

### Shadow traversal invariants

Direct-light shadow traversal is now compiled-scene aware. It receives:

- bounded shadow-caster top-level objects
- unbounded shadow-caster top-level objects
- baked composites
- baked simple bodies

Do not make `DirectLightShader` fetch scene globals through `TraceService`.
Pass the compiled structures explicitly through `RayShaderPipeline` and
`LocalSurfaceShader`, as the current code does.

Also note:

- Top-level `no_shadow` objects are filtered out by the compiled shadow lists.
- Child-level `no_shadow` is still checked through candidate attributes.
- Shadow rays use `DETAIL_NONE`; do not add normal/detail work on the shadow
  path unless filtered shadows require it.

### Case analysis for next implementation steps

Before editing code, classify the object path being touched:

| Case | Current correct route | Notes |
| --- | --- | --- |
| Top-level flat `SimpleBody`, non-CSG | `BakedSimpleBodyTracing` | Already active. Safe place for small transform/cache improvements. |
| Top-level `Composite` | `BakedCompositeTracing` | Active, but child order must match legacy exactly. |
| Nested `Composite` | Recursive `BakedCompositeTracing` | Parent index must be reserved before child bake. `kscope` exercises this. |
| `SimpleBody` with `boundingShapes` | Baked simple body plus legacy bound helpers | Bounds are still dynamic `SimpleBody` helpers; do not flatten silently. |
| `SimpleBody` with `clippingShapes` | Baked simple body plus legacy clipping helpers | Clipping point-space is easy to get wrong; use `pencil`/goldens. |
| CSG top-level or inside composite | Baked CSG traversal with conservative legacy-equivalent algorithms | Phase 5 is complete for the baked graph; only add further shortcuts as separately gated post-bake optimizations. |
| Direct-light shadow ray | Compiled top-level shadow list plus baked simple/composite helpers | `kscope` is the first guard for shadow/visibility regressions. |

### Model recommendation

Use GPT-5.5, if available, for Phase 5 and for any attempt to widen the
dedicated first-hit path beyond the currently safe flat primitive cases. Those
steps require simultaneously preserving CSG interval semantics, material
attribution, clipping/bounding behavior, shadow filtering, and candidate
ordering. GPT-5.4 has already made plausible but incorrect local changes in
this area.

GPT-5.4 is adequate for narrower follow-up work if the task is constrained to:

- documenting measured results
- adding benchmark scripts around the existing commands
- small refactors that do not change traversal order
- non-semantic cleanup of the compiled-scene container

## Additional Optimization Opportunities Not Fully Reflected In The Previous Plan

The current source review suggests several opportunities that should now be
part of the plan.

### A. Keep using bounds as first-class traversal data, not just as helper metadata

`SimpleBody::getAABB()` is already used by the current scene tracing cache and
by `RenderEngine::trace` to cull bounded top-level objects.

Plan consequence:
the new baked scene should preserve that idea and push it further, treating
world-space bounds as first-class tracing data throughout the compiled
representation, not as optional metadata attached to the parsed object tree.

### B. Split first-hit and all-crossings paths explicitly

Today the code still uses the all-crossings path as the implementation of
first-hit queries.

Plan consequence:
the baked representation should expose both APIs explicitly, instead of
reconstructing first-hit from all-crossings.

### C. Reduce repeated matrix work by storing ready-to-trace transforms

The code already stores both owner-layer and geometry-layer transforms, but the
hot path still repeatedly applies them to cloned rays and hit points.

Plan consequence:
the baked node should hold the exact transform pair that tracing needs, already
composed.

### D. Reduce transient CSG containers

`ConstructiveSolidGeometryByRaySegment.cpp` still constructs temporary
`RaySegments` and per-child crossing queues at runtime.

Plan consequence:
CSG baking should target stable child arrays, precomputed bounds, and a smaller
set of runtime merge cases.

### E. Add a cheaper scratch-ray model after baking

Even after the cheap clone constructor, `RayWithSegments` is still copied in
multiple layers.

Plan consequence:
after `BakedScene` exists, test a lighter scratch-ray or transform-stack model
for intersection-only traversal.

### F. Be careful with clipping and bounding subgraphs

`SimpleBody::doIntersectionForAllRayCrossings` still iterates through
`boundingShapes` and `clippingShapes` recursively on the hot path.

Plan consequence:
scene baking should precompile those helpers too, instead of leaving them as
fully dynamic `SimpleBody` trees.

## Commit Benchmark (`/tmp/stats.csv`)

- `ok` rows: 338
- Local-minimum criterion: a commit whose `render_seconds` is lower than the
  immediately previous `ok` commit and the immediately next `ok` commit in
  `position` order.

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

## Acceptance Criteria

The plan is successful only if all these conditions hold together:

- the current worktree is measurably faster than the baseline commit on the
  five fixed scenes
- the largest wins hold on the worst current regressions:
  `level2/spline`, `level3/ntreal/ntreal`, `level3/piece3/piece3`,
  `level2/iortest`, and `level1/shapes2`
- comparative heatmaps remain visually stable
- there is no architectural backslide into mutable geometry baking across the
  whole `environment/geometry` layer

## Recommended Execution Order

1. Add `Scene` compilation hooks and keep behavior unchanged.
2. Bake flat simple solids and precompute bounds.
3. Add top-level broad-phase culling.
4. Split first-hit from all-crossings on the baked simple-object path.
5. Bake CSG operands and child batches.
6. Re-measure and inspect heatmaps after each step.

That order minimizes risk: first flatten the data model, then exploit it for
traversal, then optimize the more delicate CSG path.

## Immediate Next Slice

The next implementation branch should be a narrow Phase-1 prototype, not
another traversal micro-optimization.

Status on 2026-06-30:

- in progress in the current worktree
- `Scene` now owns a compiled tracing container beside the parsed object arrays
- the compiled layer already classifies top-level objects as bounded vs.
  unbounded and records cached world-space bounds
- bakeable flat `SimpleBody` entries now also cache precomposed transform data,
  material/color references, and a direct geometry pointer for an alternate
  first-hit path in `RenderEngine::trace`
- direct-light shadow traversal now also consumes the compiled tracing list and
  routes bakeable `SimpleBody` entries through the same baked intersection
  helper instead of always re-entering the raw `SimpleBody` path
- tracing-structure rebuild now happens at the `Scene::setObjects()` boundary,
  avoiding a redundant second rebuild in `SceneParser::parse()`

Scope:

- add a compiled-scene container beside the current parsed-object arrays inside
  `Scene`
- build it once when parsed objects are committed into `Scene`
- start with behavior-preserving compiled records that still reference the
  existing `SimpleBody` / `Composite` / `CsgOperand` tree
- record, at minimum, the top-level object pointer, preclassified bounded vs.
  unbounded role, and the already computed world-space bounds

Explicit non-goals for that slice:

- do not change `SimpleBody::doIntersectionFirstHit` yet
- do not add another BVH or another top-level ordering heuristic yet
- do not attempt CSG-specialized traversal until the compiled representation
  exists and is benchmarked

Exit criterion:

- the renderer still produces the same images
- the compiled representation is available as a new tracing boundary in
  `environment.scene`
- timing should stay roughly neutral; a small regression is acceptable only if
  it cleanly unlocks Phase 2 baking work

Current check on 2026-06-30:

- compile gate passes
- spot renders of `level1/shapes2` and `level3/ntreal/ntreal` at `320x200`
  complete successfully through the compiled-scene path, including the baked
  direct-light shadow traversal
- the 5-scene timing panel for this exact Phase-1 variant currently measures:

| Scene | Baseline 4af1a75 (s) | Current Phase-1 variant (s) | Factor |
| --- | ---: | ---: | ---: |
| `level2/spline` | 1.989 | 0.443 | 0.223 |
| `level3/ntreal/ntreal` | 1.292 | 0.823 | 0.637 |
| `level3/piece3/piece3` | 4.112 | 2.962 | 0.720 |
| `level2/iortest` | 5.316 | 5.458 | 1.027 |
| `level1/shapes2` | 0.382 | 0.198 | 0.518 |

Reading:

- the compiled simple-body path plus the shadow-ray top-level culling are now
  paying off strongly on `spline` and `shapes2`
- adding exact bounds for closed quadrics turned `ntreal` from the remaining
  regression into one of the largest wins in the panel
- `piece3` stays clearly improved
- `iortest` regressed slightly and is now the only panel scene that still wants
  follow-up inspection
- the image/heatmap gate for this exact variant is still pending
