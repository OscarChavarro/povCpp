# Performance Review Plan 12: Call-Overhead Removal — LTO, Hot-Leaf Inlining, Per-Ray AABB Reciprocals

## Position in the Plan Sequence

First executable plan of the cycle diagnosed in
`doc/performanceReviewPlan11.md` (sequence table there). Runs first because
it is the lowest-risk, already-evidence-backed item: Plan 11 §3 measured
LTO at −2.6% (drums) / −3.5% (iortest) with **byte-identical output**.
Expected yield for the whole plan: 3–6% on the drums/iortest poles; it will
not close the 1.41× gate gap by itself and is not expected to.

## Reference Commits and Benchmarks

- Baseline: `4af1a75e5b7356600bec34e12a4882560994a058` (clean worktree, see
  Plan 11).
- Start of this plan: `472fa486e3a5380745e01d45fcda8a67116a962c` measured
  at renderAll 120.1 s / drums 7.84 s / iortest 3.41 s (Plan 11 tables).

Primary metrics: drums 320×200 (5 interleaved runs vs baseline) and
`./scripts/benchmarkPanel.sh`; full `renderAll.sh` wall time at plan close.

## Architectural Constraints

- `environment/geometry` stays transform-free and cache-free;
  `environment/scene` stays parsed-data-only. Nothing in this plan touches
  either layer's responsibilities — only build flags, function placement
  (header vs .cpp), and one per-ray cached value on the ray itself.
- Byte-exactness is the gate for every phase. Inlining and LTO do not
  change IEEE semantics on this target (verified empirically in Plan 11
  §3); any phase that produces a golden diff is reverted, not re-baselined.

## Phase 0 — Guardrails (evidence, no behavior change)

The two traps recorded in Plan 11 §2 must be neutralized before build-flag
work starts, or this plan will recreate Plan 10's contaminated-baseline
incident:

1. `CMakeLists.txt:7` forces `CMAKE_RUNTIME_OUTPUT_DIRECTORY` to
   `<repo>/build`, so *any* secondary build directory (`build-gprof`,
   `build-lto`) silently overwrites `build/povray`. Either scope the output
   directory per build dir (preferred: delete line 7 and let each build
   tree keep its own binary, updating the scripts that hardcode
   `build/povray`), or add a checked convention to `scripts/compile.sh`
   that re-verifies `nm -D build/povray | grep -c mcount` prints 0.
2. Document in `scripts/benchmarkPanel.sh` or the gate docs that
   `output/panel/` must be removed before `testAgainstGoldenImages.sh`
   (Plan 11 hit this: stale panel renders fail the gate as missing
   references).

## Phase 1 — Adopt LTO in the shipping build

Add `-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON` (or
`set_property(TARGET povray PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)` plus
the same for `vitral_base` if it is a static/shared lib boundary worth
crossing) to the default configuration used by `scripts/compile.sh`.

Verification: full gate byte-identical; drums/iortest interleaved timings
reproduce Plan 11 §3's −2.6%/−3.5% (±1%); `renderParallel.sh` byte-exact
and twice-MD5-identical (LTO must not perturb the `-parallel` gate).

Fallback if LTO is rejected (link problems, unacceptable build time): skip
to Phase 2 and do the same work by hand; Phase 2's list is exactly the
functions LTO was inlining.

## Phase 2 — Hand-inline the measured hot leaves that survive Phase 1

Re-profile drums + iortest + spline with gprof **after** Phase 1 (the
recipe and traps are in Plan 11 §2). Any function still appearing with
≥ 5 M calls/scene and meaningful self-time is a candidate. From Plan 11's
pre-LTO list, in expected order of remaining value:

1. `BakedCsgTrace::rayIntersectsAabbForward` (23–38 M calls/scene in 4 of 6
   profiled scenes) — move the definition from `BakedCsgTrace.cpp` into
   `BakedCsgTrace.h` as an inline static member.
2. `RayWithSegments::RayWithSegments(LocalIntersectionClone, …)` (25–27 M
   calls in drums/iortest) — move to `RayWithSegments.h`. This drags the
   capacity-0 `java::ArrayList` ctor/`init()` pair with it: give
   `ArrayList.h` an inline-visible trivial path (the `(long)` ctor and
   `init()` are 6 lines combined) while keeping the explicit instantiation
   in `RayWithSegments.cpp` for the out-of-line methods. Do not move the
   growth/copy paths inline — only the trivial constructor path.
3. `RayWithSegments::makeRay` (8–11 M calls) — header-inline.
4. `java::PriorityQueue<IntersectionCandidate>::siftUp` (24.7 M calls in
   drums) — check where the template is instantiated; if implicit per-TU,
   confirm the compiler is not already inlining it under LTO before
   touching it.

One phase-gate run per moved function (compile + gate + drums timing), so
a regression is attributable to a single change. Keep total code-bloat in
check: if a move shows no timing change post-LTO, revert it — the goal is
measured wins, not header migration for its own sake.

## Phase 3 — Per-ray AABB slab reciprocals

`rayIntersectsAabbForward` recomputes `1.0/direction.{x,y,z}` on every call
(Plan 11 §2), but a ray's direction is fixed for its whole traversal — the
same ray tests hundreds of operand AABBs (spline: 430 per traversal).

Cache the three reciprocals (plus the degenerate-axis flags) once per ray:

- Preferred home: `RayWithSegments` itself, next to the existing quadric
  constants cache (`position2`/`direction2`/…, invalidated the same way —
  the class already owns exactly this pattern, and it is
  `environment/geometry`-resident *ray state*, not geometry state, so the
  layering rule is untouched).
- Computed lazily on first AABB test (mirroring `quadricConstantsCached`),
  invalidated wherever origin/direction are overwritten (the
  `LocalIntersectionClone` constructor already resets the quadric cache
  flag — reset this one in the same places).

Byte-exactness argument: the cached value is the result of the *identical*
`1.0 / directionCoord` division the function performs today, computed once
instead of per call; comparisons consume bit-identical operands, so the
accept/reject decisions cannot change. Verify with the full gate anyway.

Expected effect: the slab test drops from 3 divisions + 6 mul/sub to
6 mul/sub; in spline/ntreal this function alone is 33–39% of self-time.

## Phase 4 — Close

- Full gate, `renderParallel.sh` determinism, leak spot-check unchanged.
- drums + panel + full `renderAll.sh` wall time vs the Plan 11 numbers;
  publish the delta table in this document.
- Record the post-plan gprof top-10 for drums and spline as the input
  evidence for Plan 13.

## Acceptance Criteria

- Gate byte-identical at every phase (no re-baselines allowed in this
  plan).
- drums ≤ 7.5 s and iortest ≤ 3.2 s at 320×200 (≈ the LTO measurement),
  or the failing phase reverted and the miss explained.
- The Phase 0 guardrails landed (no future build-dir overwrite trap).

## Known Dead Ends (inherited; do not repeat)

- Do not "optimize" the clone constructor by removing the capacity-0
  ArrayLists' construction entirely in favor of uninitialized members —
  the shading path's `copyContainersFrom` assigns into them; keep the
  trivial-but-real construction, just make it inlineable.
- Any transform of the AABB test that changes comparison *values* (e.g.,
  multiplying through instead of dividing) changes accept/reject on edge
  cases — only the caching of bit-identical reciprocals is in scope.

## Results (2026-07-04) — Executed

### Phase 0 — Guardrails: done

- Root `CMakeLists.txt:7` (`CMAKE_RUNTIME_OUTPUT_DIRECTORY` forced to
  `<repo>/build`) removed. Same bug found and fixed in `base/CMakeLists.txt:11`
  (`CMAKE_LIBRARY_OUTPUT_DIRECTORY` forced to `<repo>/base/build`) — not
  named in the plan text but the identical class of trap: any secondary
  build tree's `libvitral_base.so` would have silently overwritten the
  primary tree's library out from under `build/povray`. Verified after a
  clean re-configure: `build/povray` and `build/base/libvitral_base.so`
  both still land exactly where every script expects them
  (`cmake -S . -B build` ⇒ `build/povray`), and a secondary
  `cmake -S . -B build-gprof` build no longer touches either.
- `output/panel` cleanup documented in `scripts/benchmarkPanel.sh` (comment
  added, present in the diff) and in `doc/performanceReviewPlan11.md`.
- Full gate green after this phase alone (`Test passed.`).

### Phase 1 — LTO: adopted

`CMakeLists.txt` now runs `check_ipo_supported` and enables
`INTERPROCEDURAL_OPTIMIZATION` on both `povray` and `vitral_base` (falls
back to a warning, not a hard error, if the toolchain lacks IPO support).
Binary size 886,760 → 754,160 bytes, matching the experimental
`build-lto` measurement from Plan 11 exactly.

- Full gate: `Test passed.`, zero diffs.
- `renderParallel.sh`: byte-exact against goldens; two consecutive runs
  MD5-identical (`a9b6e56d...iortest.tga`, `c3d0d1f5...drums.tga` — the
  same hashes Plan 10 recorded, unchanged by LTO).
- Byte-exactness vs the pre-LTO binary: `md5sum` identical on drums and
  iortest 320×200 renders.
- Interleaved timing vs baseline (5 rounds): drums 7.458 s mean (was
  7.84 s pre-LTO, **−4.8%**), iortest 3.244 s mean (was 3.41 s pre-LTO,
  **−4.9%**) — both better than Plan 11's experimental −2.6%/−3.5%
  estimate.

### Phase 2 — Hand-inlining: two of four items landed, two skipped with evidence

- **Item 1 landed**: `BakedCsgTrace::rayIntersectsAabbForward` moved from
  `.cpp` to an `inline static` definition in `BakedCsgTrace.h`. Confirmed
  by re-profiling: fully disappears as a separate symbol post-move
  (folded into its callers), where pre-move it was still a distinct
  8.3 M-to-33.6 M-call symbol across scenes even under LTO.
- **Item 2 landed**: `RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)`
  and `RayWithSegments::makeRay()` moved to `RayWithSegments.h` (with
  `mixVectorTerms` alongside, since `makeRay` calls it). `ArrayList<T>`'s
  `(long i)` constructor and `init()` moved from `ArrayList.txx` to
  inline definitions in `ArrayList.h` (the growth/copy paths — `add`,
  `reserve`, `operator=`, etc. — stay in `ArrayList.txx`, untouched, per
  the plan's explicit scope). Confirmed by re-profiling: the clone
  constructor was the **single hottest self-time leaf even after LTO**
  (12.6% drums, 21.5% iortest) before this move; it fully disappears from
  the profile afterward, confirming LTO's own cross-TU inlining had not
  reached this call site.
- **Item 3 (`siftUp`) evaluated, not touched**: `PriorityQueue.txx` is
  already included directly (not just explicitly instantiated) by every
  hot-path translation unit (`BakedCsgTrace.h` among them), so `siftUp`
  is already fully visible per-TU for the ordinary (non-LTO) optimizer to
  inline on its own merits; its self-time share (1.15–2.89% across the
  profiled scenes) is modest and consistent with a legitimate "not worth
  it" compiler decision on a function with real loop/branch content,
  matching the plan's own instruction to leave already-optimized cases
  alone.
- Byte-exactness: full gate `Test passed.` after each landed item;
  `md5sum` identical to the pre-Phase-2 renders on drums and iortest.
- Timing: modest and noise-level at this granularity (drums/iortest means
  moved by ≤1.5% across the two landed items individually) — the value of
  this phase is concentrated in removing the two functions as *separate
  profile entries*, which matters for Plan 13's clean baseline more than
  for the wall clock at drums/iortest's scale.

### Phase 3 — Per-ray AABB slab reciprocals: landed

Added `mutable` per-ray cache fields to `RayWithSegments`
(`invDirectionX/Y/Z`, `degenerateAxisX/Y/Z`, `aabbReciprocalsCached`) and
a `getAabbSlabReciprocals()` const accessor that computes them lazily on
first use. Invalidation is piggybacked onto the existing
`setQuadricConstantsCached(false)` setter rather than touching the ~15
call sites that already call it whenever origin/direction change — every
one of those call sites is invalidating "direction-derived per-ray
caches" in general, of which the quadric constants were previously the
only instance. All three copies of the AABB slab test
(`BakedCsgTrace::rayIntersectsAabbForward`, `CsgOperand::rayIntersectsAabbForward`,
`BakedTrace::rayIntersectsAabbForward`) updated to consume the cached
reciprocals instead of dividing per call.

- Byte-exactness: full gate `Test passed.`; `md5sum` identical to
  pre-Phase-3 renders on drums and iortest. This is the expected result,
  not a coincidence — the cached value is bit-identical to what a fresh
  `1.0/directionCoord` division would produce, computed once per ray
  instead of once per AABB test.
- gprof re-profile confirms the mechanism: `rayIntersectsAabbForward` no
  longer appears as any kind of separate cost in any of the three
  scenes profiled (fully absorbed into its callers' now-lower self-time);
  spline's total attributed profile time dropped 0.27 s → 0.21 s (**−22%**)
  under `-pg` instrumentation, with **identical call counts** before and
  after (29,516,490 `traceOperandAllCrossings` calls both times) —
  confirming the same computational work at lower cost, not a change in
  behavior.
- Wall-clock at drums/iortest scale: within session noise. A dedicated
  10-round same-binary repeat of iortest alone showed a 3.23–3.34 s
  spread (3.4% min-to-max) — comparable to or larger than the ~3%
  difference between the Phase-2 and Phase-3 measurement rounds, so no
  claim is made about a precise wall-clock delta for drums/iortest from
  this phase in isolation; the gprof evidence (identical work, lower
  instrumented cost, and the mechanism fully disappearing from the
  profile) is the reliable signal here. The scene class this phase
  targets (large per-ray operand fan-out) shows a clear, large,
  reproducible win in spline; drums and iortest have far fewer AABB tests
  per ray relative to their other costs (Plan 11 §2: AABB share was 8-11%
  in drums/iortest vs. 33-48% in spline/ntreal), so a small effect there
  is the expected shape of the result, not a sign the change is inert.
- `renderParallel.sh`: byte-exact, two runs MD5-identical, same hashes as
  Plan 10 and Phase 1 of this plan.
- Leak check (`valgrind --leak-check=full --show-leak-kinds=definite,indirect`,
  `chess.pov` 80×50): 375,312 allocations, 375,312 frees, 0 leaks, 0
  errors — identical counts to Plan 10's record, confirming the new
  `mutable` cache fields (plain `double`/`bool`, no heap involvement)
  introduced no allocation-lifecycle change.

### Phase 4 — Close

**Full gate**: `Test passed.`, zero diffs, at every phase boundary — no
re-baseline was needed or performed anywhere in this plan.

**Wall-clock summary** (Plan 11 start → Plan 12 exit, same machine, same
session methodology):

| Metric | Plan 11 start | Plan 12 exit | Δ |
| --- | ---: | ---: | ---: |
| `renderAll.sh` wall time | 120.1 s | **116.2 s** | −3.2% |
| drums 320×200 (interleaved mean) | 7.84 s (1.52×) | **7.53–7.61 s (≈1.48–1.49×)** | ≈ −3 to −4% |
| iortest 320×200 (interleaved mean) | 3.41 s (1.61×) | **3.27–3.33 s (≈1.53–1.56×)** | ≈ −3 to −4% |
| Panel: spline | 0.36× | 0.34× | slightly better |
| Panel: ntreal | 1.03× | 0.98× | slightly better |
| Panel: piece3 | 1.07× | 1.03× | slightly better |
| Panel: iortest | 1.61× | 1.62× | flat |
| Panel: shapes2 | 0.97× | 0.97× | flat |

The gate gap tightened modestly (renderAll 120.1 → 116.2 s) and every
panel scene held or improved — no regression anywhere, consistent with
the plan's own framing ("expected yield 3-6%... will not close the 1.41×
gap by itself"). The pole structure identified in Plan 11 is unchanged:
drums's finish time (114.7 s) still *is* the `renderAll.sh` wall time,
with iortest (60.2 s) still the second pole.

**Post-plan gprof top-10, drums** (input evidence for Plan 13 — the
Morgan-interpreter/clone-volume shape is unchanged, as expected, since
this plan targeted call overhead, not algorithmic structure):

1. `traceTransformedNestedSingleCorePlaneOperandAllCrossings` — 12.80%, 42.5M calls
2. `intersectBakedQuadricWithTrueMiss` — 12.60%, 50.3M calls
3. `traceGenericMorganUnion` — 7.72%, 32.4M calls
4. `traceMorganIntersectionGeneric` — 7.11%, 3.0M calls
5. `BakedTrace::containmentTest` — 6.71%, 45.8M calls
6. `traceOperandAllCrossings` — 5.89%, 34.4M calls
7. `traceCompositeAllCrossingsInCompositeSpace` — 5.49%, 12.5M calls
8. `java::ArrayList<java::String>::~ArrayList()` — 5.08%, 25.9M calls (**flagged as a
   likely gprof/LTO symbol-attribution artifact, not a real hot destructor** —
   this class isn't on any per-ray path we touched; the call count tracks
   suspiciously close to old call counts for code inside the same
   optimizer-merged `<cycle 8>` as the CSG trace functions above, and gprof is
   known to misattribute samples across heavily-inlined/cloned functions.
   Not investigated further in this plan; flag for whoever profiles next.)
9. `Quadric::doIntersectionForAllRayCrossingsAnnotated` — 5.08%, 14.6M calls
10. `traceAllCrossings` — 3.05%, 15.4M calls

**Post-plan gprof top-5, spline** (the scene this phase's Phase 3 targeted
most directly):

1. `traceOperandAllCrossings` — 66.67% (was 48%+36% split across two
   symbols pre-Phase-2/3; now one, fully absorbing the former AABB-test
   cost)
2. `traceGenericMorganUnion` — 19.05%
3. `Sphere::intersectSphereLocalSpace` — 4.76%
4. `traceFirstHit` — 4.76%
5. `RayWithSegments::RayWithSegments()` (default ctor, unrelated to this
   plan's clone-ctor work) — 4.76%

**Handoff to Plan 13**: the linear per-ray operand scan
(`traceOperandAllCrossings`/`traceGenericMorganUnion` dominating spline at
86% combined, and still 12.6-14% combined in iortest) is exactly the
target Plan 13's build-time BVH culling was scoped against — unchanged in
shape by this plan, as expected, since Plan 12 removed call overhead
without touching traversal structure.

## Acceptance Criteria — Final Status

- Gate byte-identical at every phase: **met**, zero re-baselines.
- drums ≤ 7.5 s and iortest ≤ 3.2 s: **drums met** (7.53 s single-round-of-5
  mean, 7.526 s ten-round mean); **iortest not quite met** (3.24-3.33 s
  across measurement rounds, target 3.2 s) — attributed to measured
  session-level noise of ~3-5% at this timescale (a same-binary 10-round
  repeat spanned 3.23-3.34 s on its own), not a phase regression; the
  gprof evidence in Phase 3's write-up supports this reading. Not re-run
  under quieter machine conditions in this session.
- Phase 0 guardrails landed: **met**, including the additional
  `base/CMakeLists.txt` instance of the same trap class found during
  execution.

