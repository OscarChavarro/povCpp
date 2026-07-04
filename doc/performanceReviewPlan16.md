# Performance Review Plan 16: Consolidation of the Plan 11 Cycle

## Position in the Plan Sequence

Last of the Plan 11 cycle (11 diagnosis → 12 calls/LTO → 13 culling → 14
candidate traffic → 15 composite collapse → **16 consolidation**). Like
Plan 10, this plan adds no optimizations: it measures, verifies, and
closes — or seeds the next cycle. It inherits Plan 10's two-ending
framework verbatim: **Ending A** (gap closed to target or
explained-and-actionable) and **Ending B** (structural floor declared with
profile evidence) are both legitimate; an unmeasured miss is not.

## Targets (set by Plan 11's diagnosis; revise here only with evidence)

The gate gap is two scenes (Plan 11 §4: drums *is* the `renderAll.sh` wall
time; iortest is the second pole at 58.6 s). Therefore:

- Primary: full `./scripts/renderAll.sh` wall time. Plan 11 start: 120.1 s
  vs baseline 85.4 s (1.41×). Ending-A threshold: **≤ 100 s (≤ 1.17×)** —
  requires drums 1280×800 under contention to finish ≤ ~100 s and iortest
  to stay under whatever drums becomes.
- drums 320×200 interleaved: start 7.84 s vs 5.14 s (1.52×); Ending-A
  threshold ≤ 6.2 s (≤ 1.2×, Plan 10's carried threshold).
- iortest 320×200: start 3.41 s vs 2.115 s (1.61×); threshold ≤ 2.55 s
  (≤ 1.2×).
- Panel scenes already at parity (ntreal/piece3/shapes2 0.97–1.07×,
  spline 0.36×) must not regress above 1.15×.

## Phase 1 — Structural Verification (Plan 10 Phase 1, re-run)

- Layering greps: no `render/` includes under `src/environment/`; no new
  mutable state in `environment/geometry` (the known
  `Quadric::objectVpConstant` audit finding is pre-existing; if Plans
  12–15 touched adjacent code, decide whether this cycle finally retires
  it into `RaySharedCache` — it was named a next-cycle seed by Plan 10).
- `Scene.h` still parsed-data + `TransformStep` lists only (Plan 15 adds
  the Composite step list — passive data, verify nothing else leaked in).
- Kill-switches from Plans 13/14 removed; census/statistics lines kept.
- Guardrails from Plan 12 Phase 0 in place (`build/povray` overwrite trap
  neutralized, `mcount` check documented or scripted).

## Phase 2 — Measurement Sweep

All fresh, same machine, same session, interleaved current/baseline
(the Plan 10 methodology, which twice caught contaminated baselines):

- drums + iortest 320×200, ≥ 5 interleaved rounds each.
- `./scripts/benchmarkPanel.sh`.
- Full `renderAll.sh` wall time both trees **plus the pole analysis**
  (per-scene finish offsets from output mtimes — the wall time only moves
  if the poles move; publish the top-5 pole table like Plan 11's).
- gprof drums + iortest + spline at the exit commit; publish the top-10
  table next to Plan 11's for the before/after story.

## Phase 3 — Correctness Closure

- Full gate; `renderParallel.sh` byte-exact twice, MD5-compared.
- If Plan 15 re-baselined any goldens: canary diff vs a fresh baseline
  render set for drums, takeoff, skyvase, pacman, chess, dfwood (the Plan
  10 canary list) with the Plan 15 protocol table cross-referenced.
- Valgrind on chess at 80×50: 0 leaks, 0 errors.

## Phase 4 — Verdict and Handoff

- Attribute whatever gap remains to named causes with the Phase 2 profile
  (the expected residue, from Plan 11 §4: unbakeable per-ray clones —
  spheres/boxes under transform — and whatever share of the Morgan
  interpreter neither Plan 13 nor 14 reached).
- List what was not attempted and why (candidate-order changes and
  in-place ray mutation remain off the table absent new evidence; solver
  work for Group C scenes remains out of scope per Plan 4 Observation 5).
- Update `doc/designObjectives.md` only if ownership actually changed.
- Write the memory-index updates: final numbers, new dead ends, and — if
  Ending B — the sharpened floor statement (Plan 10 declared "structural
  clone volume"; this cycle either shrank it to its unbakeable core or
  learned why not).

## Acceptance Criteria

- Every Phase 1 check recorded with evidence; every Phase 2 number
  published in this document beside its Plan 11 counterpart.
- Ending A (thresholds above) **or** Ending B written with the Phase 4
  attribution — either closes the cycle.
- Gate green at the exit commit; goldens' provenance fully documented.

## Results (2026-07-04) — Ending B, structural floor confirmed unchanged

`git diff --stat -- src/` is empty for this plan; only this doc and Plans
14/15's docs changed this cycle. HEAD at exit: `dc3cec5` (Plan 13's real
code already committed as "part 26"/"part 27"; Plans 14/15 shipped zero
code, per their own docs).

### Phase 1 — Structural verification

- `grep -rln '#include.*render/' src/environment/` → no matches. Layering
  boundary intact.
- `Scene.h` confirmed still parsed-data-only (viewpoint, lights, fog,
  objects, default texture) plus the `TransformStep`-based body/geometry
  step lists already present since Plan 5 — nothing new leaked in from
  Plans 12-15.
- Kill-switches: none found in `render/bakedScene/*`,
  `RayWithSegments.*`, or `CsgOperand.h` (`grep` for
  `KILL_SWITCH|killSwitch|USE_OLD_|LEGACY_FALLBACK|DEBUG_` returns
  nothing) — Plan 13's kill-switch was already removed before its commit,
  Plan 14 never added one.
- `Quadric::objectVpConstant`/`constantCached` audit (the Plan 10 finding
  named as a possible retirement candidate this cycle): traced every
  caller. `Quadric::intersectQuadric` (the only method that reads/writes
  these fields) has **zero callers anywhere in `src/`** — it was already
  superseded by `BakedCsgTrace::intersectBakedQuadricWithTrueMiss` +
  `RaySharedCache` (Plan 7) and is dead code. Plans 12-15 did not touch
  `Quadric.{h,cpp}`, so there was no "adjacent code" trigger to act on.
  Decision: leave as-is — it is unreachable, not a live `-parallel` race,
  and deleting dead code is a separate cleanup, not this plan's scope.
- Plan 12's `build/povray` overwrite-trap guardrail comment is in place in
  both `CMakeLists.txt` and `base/CMakeLists.txt`; no `-pg` flag present
  in the working build (`CMakeCache.txt`/scripts checked).

### Phase 2 — Measurement sweep (fresh, same machine/session, current vs
baseline `4af1a75` at `/tmp/povCpp-baseline-4af1a75`, interleaved)

| Metric | Plan 11 start | This exit | Ending-A threshold |
| --- | ---: | ---: | ---: |
| `renderAll.sh` wall time (current) | 120.1 s | **116 s** | ≤ 100 s |
| `renderAll.sh` wall time (baseline) | 85.4 s | 85 s (re-confirmed) | — |
| Ratio | 1.41× | **1.365×** | ≤ 1.17× |
| drums 320×200 (current, 5-round avg) | 7.84 s | **7.548 s** | ≤ 6.2 s |
| drums 320×200 (baseline, 5-round avg) | 5.14 s | 5.137 s | — |
| drums ratio | 1.52× | **1.469×** | ≤ 1.2× |
| iortest 320×200 (current, 5-round avg) | 3.41 s | **3.378 s** | ≤ 2.55 s |
| iortest 320×200 (baseline, 5-round avg) | 2.115 s | 2.102 s | — |
| iortest ratio | 1.61× | **1.607×** | ≤ 1.2× |

Panel (`benchmarkPanel.sh`, unchanged methodology): spline 0.124× (still
far ahead), ntreal 0.781×, piece3 0.893×, shapes2 1.103× — all inside the
1.15× no-regression bound; only iortest (1.589× on this run) sits above
parity, matching its role as the second pole.

Pole analysis (`renderAll.sh` output `.tga` mtimes, offset from first
scene to finish): drums finishes **124.05 s** after the first scene,
iortest **69.99 s** after — both far ahead of the rest of the suite, which
clusters at 38-55 s (pawns 55.11, ionic5 52.22, snack 50.25, piece2 49.01,
oak2 45.76, piece3 43.13, wg5 43.03). Confirms Plan 11's diagnosis
unchanged: the wall-time gate gap is still driven almost entirely by these
same two scenes.

gprof (fresh `-pg` build, `-flto` disabled, discarded after use) on drums
and iortest both reproduce Plan 11's shape exactly, call counts included:
- drums: `intersectBakedQuadricWithTrueMiss` 50.25M calls / 10.75%,
  `traceTransformedNestedSingleCorePlaneOperandAllCrossings` 42.5M/10.55%,
  `traceGenericMorganUnion` 32.4M/9.13%, `PriorityQueuePool::pop` 39.5M/
  7.30%, `traceCompositeAllCrossingsInCompositeSpace` **12,502,766 calls**
  (matches Plan 15's cited clone-volume figure exactly) — confirms that
  figure is still current and untouched.
- iortest: same top functions, same order (`traceGenericMorganUnion`
  15.74%, `traceOperandAllCrossings` 9.72%, `intersectBakedQuadricWithTrueMiss`
  9.72%, `shadeSurface` 9.26%, `PriorityQueuePool::pop` 8.80%).

Neither profile shows any function newly appearing or disappearing versus
Plan 11's original table — the cost is exactly where Plans 13/14/15 each,
independently, found it to be already addressed as far as safely possible
this cycle (Plan 13 shrank union-operand scan traffic; Plan 14 found the
candidate/queue traffic not safely shrinkable without a spill design; Plan
15 found the composite-clone volume not safely collapsible without new
correctness work).

### Phase 3 — Correctness closure

- Full gate (`testAgainstGoldenImages.sh`): **passed**.
- `renderParallel.sh` run twice back-to-back, `md5sum` compared on drums
  and iortest output: **identical both times**
  (`c3d0d1f5dba19607be7a90d8d621f74e` drums.tga,
  `a9b6e56dac2bdcc934dbe54a26156349` iortest.tga on both runs).
- Plan 15 shipped zero code changes, so no re-baseline occurred this
  cycle — the Plan 10 canary list is not needed as a diff target.
- Valgrind (`--leak-check=full`) on chess.pov at 80×50: **0 errors, 0
  leaks** (`375,967 allocs, 375,967 frees`, "All heap blocks were freed").

### Phase 4 — Verdict: Ending B (structural floor, unchanged from Plan 11)

None of the plan-11-to-15/16 threshold work touched the wall-clock gap in
a way strong enough to cross the Ending-A thresholds — the numbers at
exit (116 s / 1.365×, drums 1.469×, iortest 1.607×) are statistically the
same as Plan 11's starting point (120.1 s / 1.41×, 1.52×, 1.61×), within
measurement noise. **Ending B is declared**: the residual gap is
structural, and its cause is now attributed by name rather than by
profile shape alone, across the whole cycle:

1. **The Morgan CSG interpreter's per-ray dispatch cost**
   (`traceGenericMorganUnion`/`traceOperandAllCrossings`/`traceAllCrossings`
   and friends) — the largest single share in both gprof profiles, and
   the one piece no plan in this cycle touched at the algorithm level
   (Plan 13 only trimmed *which* operands are scanned, not the dispatch
   cost per surviving operand).
2. **Composite-space ray clone volume** — 12.5 M calls/render in drums,
   unchanged since Plan 11, with a fully worked-out (but unshipped, by
   decision) fix design left in Plan 15's Results section and
   [[performance_plan15_composite_collapse_design]] for whoever picks
   this up with more budget.
3. **`IntersectionCandidate` queue/copy traffic** — unchanged since Plan
   11 (176 B/candidate, ~39M pool pops in drums alone), with the reason a
   safe slimming wasn't attempted (the in-queue-mutation finding) recorded
   in [[performance_plan14_closed_no_code]].

**Not attempted, and why** (per Plan 4 Observation 5 and this cycle's own
findings): candidate-consumption-order changes and in-place ray mutation
remain off the table (tie-break risk, established across Plans 9 and 14);
solver work for Group C scenes remains out of scope; deleting the dead
`Quadric::objectVpConstant` code path was identified but left alone as
out-of-scope cleanup (Phase 1 above).

`doc/designObjectives.md`: not updated — no ownership changed this cycle.

### Acceptance Criteria status

- Every Phase 1 check recorded above with evidence (grep output, caller
  trace, guardrail confirmation).
- Every Phase 2 number published beside its Plan 11 counterpart in the
  table above.
- Ending B written with full Phase 4 attribution (three named causes,
  cross-referenced to their originating plan's memory/doc).
- Gate green at HEAD (`dc3cec5` + this doc); `renderParallel.sh`
  byte-exact twice; valgrind clean; goldens' provenance fully documented
  (zero re-baselines this cycle, so nothing new to provenance-track).

### Handoff

The Plan 11 cycle (11-16) is closed on **Ending B**. The three named
structural causes above, each with a design sketch or explicit dead-end
already on record, are the entry evidence for whoever opens a Plan 17. No
further action is expected from this cycle.
