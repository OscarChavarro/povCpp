# Performance Review Plan 10: Consolidation — Full-Suite Re-Profile and Baseline Comparison

## Position in the Plan Sequence

Last of six (Plans 5–10; sequence table in `doc/performanceReviewPlan5.md`).
Requires Plans 5–9 closed (each with its own accepted gate state — for
Plans 8 and 9, "closed" includes the re-scoped outcomes: Plan 8's retired
fusion phases with the refocused R-phases resolved either way, and Plan 9's
possible SKIPPED verdict). This plan adds no optimizations: it measures,
verifies the structural claims made across the sequence, closes remaining
bookkeeping, and defines what, if anything, justifies a further cycle.

## Reframing (2026-07-04, after the Plan 7/8 findings)

The sequence's accumulated evidence changed what this plan should verify.
Plans 5-8 delivered the structural goals (decoupling, rebuilt baked model,
per-ray invariants) and real correctness wins (two Plan 5 numeric bugs, the
Plan 7 `-parallel` viewpoint-cache data race), but four consecutive
dispatch-level optimizations produced no measurable drums improvement, and
drums sits at ~1.7× the (unrevalidated) baseline figure. Plan 10 therefore
has two explicitly legitimate endings, decided by evidence, not preference:

- **Ending A — gap closed or explained-and-actionable:** Plan 8-R's
  collapse-rate expansion moved drums materially; the residue has named,
  actionable causes seeding a next cycle.
- **Ending B — structural floor declared:** the profile shows the remaining
  gap is arithmetic volume inherent to evaluating this scene population
  under the decoupled architecture (the specialization the baseline used is
  *illegal* under the ownership rules this project chose deliberately —
  Plan 4 Conclusions §6 said exactly this). The cycle closes with the floor
  documented and the correctness/architecture wins as the deliverable.
  **Declaring Ending B honestly is success, not failure**; what is
  prohibited is declaring it without the Phase 2/3 measurements or using it
  to skip them.

## Objectives Being Verified (mapping back to the cycle's high-level goals)

1. **Geometry layer kept simple; geometry/baking decoupling intact.**
2. **`render/bakedScene` fully rebuilt** and owning a truly baked model
   (math baked, not routing metadata; nothing constant-across-rays
   re-executed per ray).
3. **Performance accounted for against the baseline `4af1a75`** — either
   recovered (Ending A) or attributed to named structural causes with
   profile evidence (Ending B). The original phrasing "as close as possible
   to the baseline" stands, with the Plan 7/8 record defining what
   "possible" has turned out to mean.

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

| Metric | Baseline `4af1a75` | Plan 4 state `0d2a21c` | 2026-07-04 (`f3ac202`, drums) | Plan 9 exit |
| --- | --- | --- | --- | --- |
| Functions to reach 98% self-time | 3 hold 62% | 93 | ~40 (top 7 hold ~54%) | *(measure)* |
| CSG-D + scaffolding share | ~13% | 47.2% | ~45% | *(measure)* |
| Prim math share | ~40% | 18.5% | ~16% (12.5% in one baked-quadric helper) | *(measure)* |
| Queue share | ~8% | 11.7% | ~2% (siftUp+add) | *(measure)* |
| `traceOperandAllCrossings` calls | n/a | 244.5 M (suite) | 53.4 M (drums alone) | *(reduced via Plan 8-R; the original "0 — function deleted" criterion is retired with the fusion phases)* |
| `LocalIntersectionClone` calls | 0 | 260.9 M (suite) | 48.6 M (drums alone) | *(residual only)* |

(The 2026-07-04 column is drums-only, not suite-wide — comparable in shape,
not in absolute counts, to the two suite columns; the Phase 2 sweep
produces the properly comparable suite table.)

## Phase 3 — Wall-Clock Panel

- drums 320×200: ≥ 5 runs at the Plan 9 exit commit **and** ≥ 5 runs of a
  freshly built `4af1a75` on the same machine, same session (the 5.10 s
  figure must be revalidated, not quoted).
- `./scripts/benchmarkPanel.sh` (5-scene panel) at both commits.
- Full `./scripts/renderAll.sh` wall time (1280×800) at both commits, plus
  the `-parallel` variant at the current commit.
- Publish the ratio table. **Both baselines must be freshly measured in the
  same session** — the Plan 6 cycle recorded an absolute-timing scare that
  turned out to be environmental, so no historical absolute number (5.10 s
  included) may be quoted without re-measurement on the current machine.
- Targets, revised 2026-07-04: drums ratio ≤ 1.2× remains the Ending-A
  threshold (the original ≤ 1.1× predates the Plan 7/8 findings; current
  standing is ~1.7× against the *unrevalidated* baseline figure). Panel
  scenes each ≤ 1.15× (currently all at 0.17-0.66× — the panel already
  beats baseline; the gap is drums-shaped, not general). If drums misses
  the threshold after Plan 8-R, Phase 5 applies the Ending-B framework
  from the Reframing section: attribute the residue with profile evidence
  and close, or name the concrete next-cycle seed.

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
  evidence. The suspect list, updated by the Plan 7/8 record: (a) residual
  transformed/nested operand population that Plan 8-R could not collapse
  (measure via the Phase R0 categorization), (b) per-test arithmetic volume
  in the baked-quadric helpers reached with `sharesRaySpace=false`, (c)
  clone/container residue if Plan 9 was skipped or partial. Retired
  suspects (measured, not guilty on this tree): dispatch/routing depth,
  union-kernel dispatch, viewpoint-constant recomputation — four gate-green
  changes across Plans 7-8 targeted these and none moved drums.
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
- drums ≤ 1.2× revalidated baseline (Ending A), **or** the Ending-B verdict
  written with the Phase 5 attribution table — either closes the cycle;
  an unmeasured or unattributed miss does not.
- Final reference state (goldens + accepted diffs + performance numbers)
  recorded as the baseline for all future performance work.
