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
