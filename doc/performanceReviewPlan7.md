# Performance Review Plan 7: `render/raySharedCache` — Per-Ray Shared Aggregate Caches Outside Geometry

## Position in the Plan Sequence

Third of six (Plans 5–10; sequence table in `doc/performanceReviewPlan5.md`).
Requires:

- Plan 5: quadric/sphere/plane operands are world-space baked copies, so one
  ray sees one coordinate space for the vast majority of tests.
- Plan 6: the rebuilt `render/bakedScene` layer with `BakedCsgProgram`
  operand tables and residual transformed operands grouped by transform
  class.

## Motivation (from Plan 4, Conclusions §2)

The baseline computed the quadric aggregate vectors — `position2` (Xo²,Yo²,Zo²),
`direction2`, `positionDirection`, and three mixed-term vectors, ~30
multiplications — **once per ray** in `RayWithSegments::makeRay`, and every
quadric, plane, and CSG operand in the scene reused them. A per-viewpoint
constant cache (`objectVpConstant`) added a second layer for primary rays.

The current branch recomputes all six aggregates on every baked-quadric call
(62.5 M + 34.1 M + 10.5 M calls in the Plan 4 sweep, ~45 mul each), because
operands lived in private local spaces and the cache key would have been
(ray × transform). Plan 5 removed the private spaces for bakeable kinds, so
the (ray)-keyed cache is **shareable again** — but it must be rebuilt
*outside* `environment/geometry`, as render-owned state.

Arithmetic target per transformed-quadric test: from ~110+ FLOPs (current)
back to ~25 FLOPs (cached aggregates + dots), the single largest remaining
per-test gap after Plan 5.

## Architectural Constraints

- New package `src/render/raySharedCache/`. Nothing in
  `environment/geometry` gains cache state or cache awareness.
  `RayWithSegments` already carries `position2` / `positionDirection`
  aggregates and `makeRay()` (`RayWithSegments.h:30-32, 99`) — that is
  geometry-owned per-ray state the baseline design used; this plan may read
  what exists but must not extend the geometry class. All *new* cache layers
  (viewpoint constants, transform-class local rays, per-class aggregates)
  live in the render package.
- Cache lifetime is per render task (tile), like the scratch context, so
  `-parallel` stays race-free. No `mutable` fields on shared baked records:
  the existing `planeVpNormDotOrigin` / `planeVpCached` mutable pair on the
  old `BakedCsgOperand` (`Scene.h:118-119`) was tolerable while
  single-threaded per scene, but the rebuilt model must route viewpoint
  caches through the task-owned cache object instead. (B6 in the parallel
  plan already showed how mutable shared state bites.)

## Design

```
src/render/raySharedCache/
    RaySharedCache.h / .cpp
```

`RaySharedCache` is created per render task and passed (by reference,
alongside the scratch context) through the `BakedTrace` entry points:

1. **World-space aggregate layer.** Keyed by ray identity (generation
   counter bumped in `BakedTrace` when a new `RayWithSegments` starts
   tracing — primary, shadow, reflected, transmitted each bump it).
   Holds the six aggregate vectors. Quadric kernels ask the cache; on
   generation miss they compute once and store. If profiling shows the
   existing `RayWithSegments::makeRay` aggregates already cover part of
   this, reuse them for that part rather than duplicating — measure, don't
   assume.
2. **Viewpoint constant layer.** Per (shape × frame) constant terms for
   primary rays, replacing the Phase E viewpoint cache and the mutable plane
   fields: an array indexed by baked-object/operand index, sized at build
   time, owned by the task cache (or per-frame if the renderer guarantees
   primary-ray single ownership; decide from the `-parallel` tile model and
   document).
3. **Transform-class layer** (residual transformed operands only — Box,
   Torus, non-bakeable nested). Plan 6 grouped these by transform class with
   a class index. Per (ray generation × class index), cache the transformed
   local origin/direction and, for classes whose members are quadrics that
   somehow remained transformed, the local aggregates. One transform per
   ray per class instead of one per ray per operand. This is Plan 4 §6.2
   verbatim, now with the class table prebuilt by Plan 6.

Sizing: class count and object count are known at build time; the cache is
flat arrays plus a generation stamp per slot — no hashing, no allocation on
the hot path.

## Phases

### Phase 1 — Cache skeleton and generation plumbing

Introduce the package, the generation counter, and thread it through
`BakedTrace` next to the scratch context. No consumer yet.
Gate: byte-identical, timing-neutral (assert with 3 drums runs).

### Phase 2 — World-space aggregate layer for baked quadrics

Consume in the baked-quadric intersection helpers (the Plan 6 ports of
`intersectBakedQuadric*`). Verify with gprof that aggregate recomputation
disappears (the helper's self-time per call drops; call counts unchanged).

Numeric caution: computing an aggregate once and reusing it must be
bit-identical to computing it at each operand *if the inputs are identical* —
they are, for world-space operands sharing the ray. Byte-identical gate is
therefore required, not merely hoped for.

### Phase 3 — Viewpoint constant layer

Port/replace the Phase E cache and the plane `planeVp*` mutable fields into
the cache object. Gate: byte-identical.

### Phase 4 — Transform-class layer for residual operands

Only if the Plan 6 builder statistics show a scene population where it
matters (check `drums`, `chess`, `pacman`, `takeoff`, `desk` counts). If no
benchmark scene has ≥2 operands per transform class, implement only the
per-(ray × class) local-ray cache (saves the clone/transform per extra
visit) and skip the aggregate part — record the decision with the
statistics.

Gate: byte-identical.

### Phase 5 — Closing measurement

drums (3 runs), panel, gprof capture, `-parallel` smoke run under
ThreadSanitizer or at minimum a repeated-render determinism check (two
`-parallel` renders byte-compared) to prove the cache is race-free.

## Measurement Gate

Every phase:

```bash
./scripts/clean.sh
./scripts/compile.sh
./scripts/renderAll.sh
./scripts/testAgainstGoldenImages.sh
```

Byte-identical output vs the Plan 6 exit state required at every phase (this
plan re-orders *when* identical math happens, never *what* it computes).
Plus drums timing at every consumer phase; panel + gprof at Phases 2 and 5.

## Acceptance Criteria

- drums wall-clock: measurable reduction; target ≥ 10% off the Plan 6 exit
  time (the aggregate recompute was ~45 mul × ~100 M calls in the old
  profile; the exact share after Plans 5–6 must be re-measured at Phase 1
  and the target restated against it in this file).
- gprof: baked-quadric helper self-time per call drops; no new function
  appears above 2%.
- No `mutable` caching fields remain on shared baked records (code review).
- `-parallel` determinism check passes.

## Status (Phases 1-3 complete, Phase 4 deferred pending decision)

Implemented `src/render/raySharedCache/RaySharedCache.h` and threaded it
through `BakedTrace`/`BakedCsgTrace` (public entry points plus every internal
function that reaches `intersectBakedQuadric*`/`intersectBakedPlane`), through
`RenderWorker` (one instance per render task, matching the `-parallel` tile
model) and through `TraceService` so `DirectLightShader`'s shadow-ray path
(which calls `BakedTrace` directly, not through `RenderEngine::trace`) can
reach the owning worker's cache too.

Investigation before implementing changed the design from what the plan
above describes:

- **Layer 1 (world-space aggregate reuse) needed no new storage.**
  `RayWithSegments` already caches its own six aggregate vectors
  (`position2`/`direction2`/.../`quadricConstantsCached`, reset by
  `setQuadricConstantsCached(false)` at every ray-generation call site: new
  primary/reflected/transmitted/shadow rays, and every local-space clone).
  `intersectBakedQuadric`/`WithTrueMiss`/`WithCoeffs` now take a
  `sharesRaySpace` bool (true when the caller's `origin`/`direction` are
  `ray->getOrigin()`/`getDirection()` verbatim, i.e. no per-operand
  transform) and read `ray->makeRay()`'s cached vectors in that case instead
  of recomputing them - no generation counter needed since the ray's own
  flag already is one.
- **Layer 2 (viewpoint constants) is frame-lifetime, not ray-lifetime.**
  `RaySharedCache` holds flat `quadricViewpointSlot`/`planeViewpointSlot`
  arrays (slot indices assigned once in `BakedSceneBuilder::build`, one per
  quadric/plane-bearing `CsgOperandRecord`/`TraceableObject`), replacing
  `Quadric::objectVpConstant`/`constantCached` and
  `CsgOperandRecord::planeVpNormDotOrigin`/`planeVpCached`. Those old fields
  were `mutable` state on baked records shared by every render-task thread
  under `-parallel` - a real, previously-unflagged data race (same class as
  the B6 noise-stats race), not merely a style problem. They are gone from
  `BakedScene.h` entirely.
- Layer 3 (transform-class local-ray cache) was **not implemented**: see
  below.

**Gate: green.** Full `renderAll.sh` + `testAgainstGoldenImages.sh` pass
byte-identical. `-parallel4` determinism re-checked directly on drums (two
runs, `compare -metric AE` = 0).

**Timing: no measurable win on drums** (3-run average ~8.2s before this
change, ~8.6s after - within run-to-run noise but not the hoped-for
improvement, and if anything slightly worse). gprof on drums shows why:
67.8M quadric tests, but the overwhelming majority go through
`intersectBakedQuadricWithTrueMiss`/`intersectBakedQuadric` with
`sharesRaySpace=false` (CSG operands transformed into local space - drums
reports 244 residual transformed operands vs only 150 collapsed/direct at
build time), so Layer 1's win does not reach most of drums's quadric tests.
Layer 2's array-indexed lookup (`RaySharedCache::getQuadricViewpointConstant`)
is a small constant-factor *regression* versus the raw mutable-field read it
replaced, on the (minority) calls where it does apply - a fair trade for
removing a real thread-safety bug, but not a speed win by itself. This
reproduces the drums characterization from Plans 5-6: the scene's cost is
structural (dominated by transformed CSG operands), not per-ray recompute of
already-shared state.

**Phase 4 decision:** the plan's own gate ("only if Plan 6 builder statistics
show a scene population where it matters") is satisfied for drums (244
transformed vs 150 direct), and Phase 4 (per ray×transform-class cached
local ray, avoiding the transform re-derivation for the 244 residual
operands) is the more plausible place left in this plan to move drums's
number. It was not attempted in this pass: it is a materially larger and
riskier change than Phases 1-3 (new per-class keying, cache-invalidation
subtleties flagged as the plan's own top risk), and is left for a follow-up
decision rather than rushed.

## Status update: Phase 4 attempted (scoped down), target not met on drums

Implemented the safe, low-risk slice of Phase 4: `traceMorganIntersectionGeneric`
(generic-Morgan `INTERSECTION` CSGs) ran its prescan and its main candidate
loop as two independent passes over the same operands, each doing its own
`localToObject` transform and quadric solve for every `TransformedQuadric`
operand - a genuine, deterministic duplicate within one ray's evaluation of
one CSG program. Fixed by caching the prescan's transformed origin/direction
and depth1/depth2 in a **plain stack array scoped to that one function call**
(indexed by loop position, capped at `MAX_CACHED_QUADRIC_OPERANDS = 64`,
falling back to the original always-correct double-trace above the cap) and
reusing it in the main loop instead of re-deriving it. This is deliberately
*not* the general "group operands sharing a transform matrix" cache the plan
sketches, and *not* a `RaySharedCache`-based cross-call cache: both of those
need a generation stamp valid across separate `BakedTrace`/`BakedCsgTrace`
entry calls, which is exactly the risk the plan itself calls out ("stale
reads... silently wrong on exactly one scene"). A stack-local cache whose
lifetime is provably bounded to the one call that fills it cannot go stale,
so it was preferred even though it only pays off this one specific
duplicate-compute pattern.

Gate: byte-identical (full suite), `-parallel4` determinism re-checked on
drums (AE=0).

**drums timing: unchanged** (~8.7s, no measurable difference from the
Phase 1-3 state). Re-profiled with gprof to find out why:
`traceMorganIntersectionGeneric` is called only ~3.0M times in drums, while
the dominant quadric-test volume (50.1M `intersectBakedQuadricWithTrueMiss`
calls, ~25% of total runtime) flows through `traceSingleCorePlaneIntersection`
/ `traceTransformedNestedSingleCorePlaneOperandAllCrossings` - the
`SingleCorePlaneIntersection`/`core-plane` specialization (250 of drums's
450 CSG programs), not the generic-Morgan-intersection path this pass
targeted (only 200 of 450, and most of those are `UNION`, which uses
`traceGenericMorganUnion` and never had the duplicate-compute pattern).
Those hot functions call the intersection helpers *once* per operand per
ray already (no analogous double-trace to eliminate); the only way to cut
their cost further is a genuine cross-call, generation-stamped cache - the
harder, riskier design the plan describes and this session deliberately did
not attempt.

**Conclusion for this plan**: Phases 1-4 are complete, correct, and
gate-green, and Phase 1-3 fixed a real `-parallel` data race, but the
plan's ≥10% drums wall-clock target is not met and is unlikely to be
reachable without the generation-stamped cross-call cache design - a
meaningfully riskier follow-on than anything implemented so far. Recommend
treating Plan 7 as closed at this state (ship the correctness fix and the
free wins) and revisiting the core-plane path's cost under Plan 8 (fused
kernels), which restructures that exact code path anyway and may make a
safe cache placement more obvious there.

## Risks and Dead-End Reminders

- Do not put the cache in `environment/geometry` or on `RayWithSegments`
  (that class is geometry-owned; extending it re-couples the layers this
  whole effort decoupled).
- Generation-stamp bugs produce *stale reads* that are silently wrong on
  exactly one scene — the byte-identical gate at every phase is the
  detector; never batch Phases 2–4 into one gate run.
- The old dead end "viewpoint cache added to all baked quadric helpers
  (Phase E)" recovered only the constant term — do not confuse re-porting
  it (Phase 3) with the new aggregate layer (Phase 2); they are separate
  wins and must be measured separately.
