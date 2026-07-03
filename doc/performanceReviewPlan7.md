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
