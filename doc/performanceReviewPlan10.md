# Performance Review Plan 10: Consolidation — Full-Suite Re-Profile and Baseline Comparison

## Position in the Plan Sequence

Last of six (Plans 5–10; sequence table in `doc/performanceReviewPlan5.md`).
Requires Plans 5–9 closed (each with its own accepted gate state). This plan
adds no optimizations: it measures, verifies the structural claims made
across the sequence, closes remaining bookkeeping, and defines what, if
anything, justifies a further cycle.

## Objectives Being Verified (mapping back to the cycle's high-level goals)

1. **Geometry layer kept simple; geometry/baking decoupling intact.**
2. **`render/bakedScene` fully rebuilt** and owning a truly baked model
   (math baked, not routing metadata; nothing constant-across-rays
   re-executed per ray).
3. **Performance as close as possible to — or better than — the baseline
   `4af1a75` (~5.10 s on drums 320×200).**

## Phase 1 — Structural Verification

Code-level checks, each recorded with its evidence in this document:

- Layering: no `render/` includes anywhere under `src/environment/`;
  no baking or cache state in `environment/geometry`:

  ```bash
  grep -rn '#include "render/' src/environment/ || echo "layering ok"
  grep -rn 'mutable' src/environment/geometry/ | grep -v '// *audited' || true
  ```

- `Scene.h` contains parsed data + `TransformStep` lists only (visual
  review; the `Baked*` structs are gone since Plan 6 Phase 4).
- Per-ray invariants: audit the hot path from `RenderEngine::trace` down to
  primitive math and confirm (a) exactly one kind switch per (ray × object),
  (b) zero per-ray material resolution, (c) zero per-ray transform of
  bakeable operand kinds, (d) staging containers allocated per task, not per
  ray. Record the call-path diagram (the successor of Plan 4's §3 diagram).
- Builder statistics across all 108 scenes: collapsed vs residual operand
  populations, fallback-kernel population (target ≈ 0), transform-class
  counts.

## Phase 2 — Full 108-Scene gprof Sweep (Plan 4 methodology, repeated)

Reproduce the Plan 4 sweep exactly so the tables are comparable:

- `-pg` Release build; each scene at 320×200 in an isolated directory
  (scenes with local includes — `drums.inc`, `ntreal.inc`, `fish.inc`,
  `teapot.inc`, `mmshape.inc`, `light.inc`, `axisbox.inc` — and data files
  — `bumpmap_.gif`, `plasma2.gif`, `pacman.iff`, `fract003.gif`,
  `eight_.iff` — run from their own directories);
- aggregate flat profiles; produce the successor of Plan 4's 98%-coverage
  table and category summary;
- rebuild the Release binary afterwards (`./scripts/compile.sh`).

Comparisons to publish side by side:

| Metric | Baseline `4af1a75` | Plan 4 state `0d2a21c` | Plan 9 exit |
| --- | --- | --- | --- |
| Functions to reach 98% self-time | 3 hold 62% | 93 | *(measure)* |
| CSG-D + scaffolding share | ~13% | 47.2% | *(measure; target ≤ 20%)* |
| Prim math share | ~40% | 18.5% | *(measure; should rise toward 40%)* |
| Queue share | ~8% | 11.7% | *(measure)* |
| `traceOperandAllCrossings` calls | n/a | 244.5 M | 0 (function deleted) |
| `LocalIntersectionClone` calls | 0 | 260.9 M | *(residual only)* |

## Phase 3 — Wall-Clock Panel

- drums 320×200: ≥ 5 runs at the Plan 9 exit commit **and** ≥ 5 runs of a
  freshly built `4af1a75` on the same machine, same session (the 5.10 s
  figure must be revalidated, not quoted).
- `./scripts/benchmarkPanel.sh` (5-scene panel) at both commits.
- Full `./scripts/renderAll.sh` wall time (1280×800) at both commits, plus
  the `-parallel` variant at the current commit.
- Publish the ratio table. Cycle target: drums ratio ≤ 1.1×, panel scenes
  each ≤ 1.15×, with "≤ 1.0× (better than baseline)" the stretch outcome —
  plausible because the current branch also carries wins the baseline lacks
  (tile parallelism aside, e.g. plan specializations the baseline never had).
  If a target is missed, Phase 5 decides whether the residue is structural
  (documented, cycle closes) or actionable (next cycle's plan 11 seed).

## Phase 4 — Correctness Closure

- Full gate at the exit commit; publish the final accepted-diff table (the
  successor of the Plan 4 / Plan 5-protocol tables) as the reference state
  for future work.
- Image comparison vs freshly rendered `4af1a75` output for the historic
  canaries: `drums` (the old accepted `AE=71` should have *shrunk* after
  Plan 5's baseline-identical numerics — verify and record), `takeoff`,
  `skyvase`, `pacman`, `chess`, `dfwood`.
- `-parallel` determinism: two full parallel renders byte-compared.
- Leak check on one CSG-heavy scene (`valgrind` or the project's usual
  procedure) — the rebuilt layer owns value-storage baked copies; confirm
  no leak and no double-free at scene teardown.

## Phase 5 — Residual Analysis and Cycle Closure

- Attribute any remaining gap over baseline to named causes with profile
  evidence (candidate suspects if the targets are missed: icache from kernel
  multiplicity, residual transformed operands, side-car model cache
  footprint, shading-layer differences).
- Explicitly list what was *not* attempted and why (e.g., candidate-order
  changes rejected for byte-exactness; polynomial solver work out of scope
  per Plan 4 Observation 5 — Group C scenes are genuine solver work,
  unrelated to the baked-CSG gap).
- Update `doc/designObjectives.md` if the ownership description changed
  (baked model now render-owned, not scene-owned — Plan 6 Phase 4).
- Write the memory-file updates for the project memory index (what the new
  layer is, where the caches live, the final numbers, the new dead ends
  discovered during Plans 5–9).
- Verdict: cycle closed, or a scoped seed list for a next cycle.

## Measurement Gate

This plan is itself a gate; the commands are those of Phases 2–4. Any code
change made during this plan (statistics, audit asserts, doc fixes) must
individually pass:

```bash
./scripts/clean.sh
./scripts/compile.sh
./scripts/renderAll.sh
./scripts/testAgainstGoldenImages.sh
```

with byte-identical output — no functional changes belong in Plan 10.

## Acceptance Criteria

- All Phase 1 structural checks pass with evidence recorded.
- Sweep, panel, and correctness tables published in this document.
- drums ≤ 1.1× revalidated baseline (target) with the miss/residual analysis
  rule of Phase 3 applied honestly if not met.
- Final reference state (goldens + accepted diffs + performance numbers)
  recorded as the baseline for all future performance work.
