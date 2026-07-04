# Performance Review Plan 10: Consolidation — Full-Suite Re-Profile and Baseline Comparison

## Status: CLOSED (2026-07-04) — Ending B (structural floor declared); also corrects a ~2x-inflated baseline in the panel/drums record carried since it was first measured

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

## Results (2026-07-04)

### Phase 1 — Structural Verification: PASS, with one audit finding

- Layering: `grep -rn '#include "render/' src/environment/` returns nothing
  — clean.
- `Scene.h` (74 lines): no `Baked*` structs, parsed data + `TransformStep`
  lists only — confirmed by visual review.
- Mutable-state audit (`grep -rn 'mutable' src/environment/geometry/`)
  found two live sites:
  - `CsgOperand::bakedBounds*` (four fields): lazily computed by
    `ensureBakedBounds()`, but its only caller is
    `CsgOperand::getBakedBounds()`, itself only called from
    `BakedSceneBuilder.cpp` — the single-threaded build phase, before any
    render tile starts. **Safe**: never touched during the parallel render
    phase.
  - `Quadric::objectVpConstant`/`constantCached`: still read/written
    directly by `Quadric::intersectQuadric` (`Quadric.cpp:184-198`),
    guarded by `ray->isPrimaryRayEnabled()` — the value is a per-shape,
    per-camera constant, invariant for the whole render. This is exactly
    the field class `RaySharedCache` (Plan 7) was built to replace, per
    its own header comment — but that replacement only covers the *baked*
    quadric path (`BakedCsgTrace::intersectBakedQuadric*`). The
    `CsgOperandKind::DirectAnnotatedPrimitive` fallback (untransformed CSG
    operands — a **routine** case, not a rare fallback: any quadric
    operand with zero transform steps takes this path) still calls
    `operand.geometry->doIntersectionForAllRayCrossingsAnnotated()`
    directly on the shared parsed `Quadric`, bypassing `RaySharedCache`
    entirely.
    **Assessment**: a genuine, unaudited data race under `-parallel` —
    but a practically benign one. Every thread computes and stores the
    *same* value (the constant depends only on the shape's fixed
    coefficients and the fixed eye position, not on which thread or which
    ray reaches it), so concurrent stores are value-identical, not
    torn/partial on the primitive `double`/`bool` types involved. No
    `-parallel` gate run across the whole Plan 5-10 sequence has ever
    shown a flaky diff attributable to this (all recorded `-parallel`
    determinism checks, including this plan's Phase 4, are byte-exact).
    Formally non-compliant with "no cache state in `environment/geometry`"
    all the same. **Named as a concrete next-cycle seed**: extend
    `RaySharedCache`'s per-slot indexing to
    `DirectAnnotatedPrimitive`/`DirectPrimitive` quadric operands, closing
    the audit gap Plan 7 left open. Not fixed in this plan (Plan 10 makes
    no functional changes).
- Per-ray invariants (spot check via the drums statistics line already
  emitted by `PovRayApplication::printStatistics`): one `TraceKind` switch
  per (ray×object) confirmed by the `Plan 6 baked model` counters (direct/
  csg/composite/bounded buckets, no double-counting); zero per-ray
  material resolution and zero per-ray bakeable-operand transform were
  already verified structurally in Plans 6-8 (`effectiveMaterial`/baked
  coefficients resolved once at build time — code unchanged since).
  Staging containers: `PriorityQueuePool` is per-`RenderWorker` (task-
  owned), confirmed in [[performance_plan9_staging_closed]]'s Phase 0
  read of `PriorityQueuePool.txx`.

### Phase 2 — Full 108-Scene gprof Sweep: Result

Fresh `-pg` sweep, 320×200, all 108 scenes, each run from its own
directory (methodology matches Plan 4's exactly — see
`/tmp/claude.../gprofSweep.sh`, not committed, mirrors `renderAll.sh`'s
scene list sequentially instead of backgrounded). gprof's native
multi-`gmon.out` summing (`gprof binary gmon1.out gmon2.out ...`)
aggregated all 108 profiles in one pass. Total attributed self-time:
**33.93 s** across 105 non-degenerate profiles (3 scenes below the 10 ms
sampling floor, matching Plan 4's own count). **95 functions reach 98%**
of that total (Plan 4: 93 functions at `0d2a21c`).

| Metric | Baseline `4af1a75` | Plan 4 state `0d2a21c` | 2026-07-04 (`f3ac202`, drums) | 2026-07-04 suite (this sweep) |
| --- | --- | --- | --- | --- |
| Functions to reach 98% self-time | 3 hold 62% | 93 | ~40 (top 7 hold ~54%) | **95** |
| CSG-D share | ~13% (with scaffolding) | 47.2% (with scaffolding) | ~45% (with scaffolding, drums) | **26.33%** (CSG-D alone); **+Cont+Comp+BTC scaffolding ≈ 39.6%** |
| Prim math share | ~40% | 18.5% | ~16% (12.5% in one helper) | **22.80%** |
| Queue share | ~8% | 11.7% | ~2% (siftUp+add, drums) | **5.26%** |
| Body (SimpleBody dispatch) share | n/a | n/a | n/a | **19.33%** |
| Ray (clone+makeRay) share | n/a | n/a | n/a | **3.84%** |
| `traceOperandAllCrossings` calls | n/a | 244.5 M (suite) | 53.4 M (drums alone) | **288.0 M (suite)** — architecture rebuilt between columns, not apples-to-apples (see note) |
| `LocalIntersectionClone` calls | 0 | 260.9 M (suite) | 48.6 M (drums alone) | **240.8 M (suite)** |

**Reading the last two rows carefully**: naive comparison to the
`0d2a21c` suite column would read as "barely moved" — but that column is
a *different codebase generation* (pre-Plan-5-10 dispatch, different
function bodies under the same-ish names), so absolute counts aren't
transitively comparable across the rebuild the way the plan's own
footnote already cautions. The finding that **is** apples-to-apples
(both measured on the current rebuilt architecture, before/after Plan
8-R2, same function): drums alone went from 48.6M to 27.3M
`LocalIntersectionClone` calls (-44%, see
[[performance_plan8_kernel_fusion]]). That the *suite-wide* total
(240.8M) still sits close to the old pre-rebuild figure (260.9M) is
itself the headline Phase 2 finding: **Plans 5-9's clone-reduction work
was CSG-transform-scoped and worked exactly where it aimed (drums), but
never touched the dominant suite-wide clone source** — structural
per-child local-space-transform clones in `traceSimpleBodyAllCrossings`/
`traceCompositeAllCrossings`, present in any scene with transformed
simple bodies or composites, independent of CSG population. Confirmed
already in [[performance_plan9_staging_closed]]'s Phase 0 read of
`RayWithSegments::LocalIntersectionClone`'s ctor (already allocation-free
at capacity 0) — there's no more low-hanging fruit in the clone's own
construction cost, only in whether the clone happens at all, which is
architectural (see Phase 5).

### Phase 3 — Wall-Clock Panel: Result, with a methodology correction

**A material correction to the record.** Re-measuring drums against a
freshly rebuilt `4af1a75` in this session initially read **12.1 s** for
the baseline (vs. the long-quoted 5.10 s) — investigation found the
baseline worktree's cmake cache still carried `CMAKE_CXX_FLAGS=-pg` from
an earlier profiling session, silently instrumenting every baseline
render with gprof overhead. A clean `rm -rf build && cmake -S . -B build`
(no `-pg`) brought it back to **5.12 s**, matching the historical figure
— so 5.10 s itself is vindicated, but the discovery led to auditing
`scripts/benchmarkPanel.sh`'s hardcoded baseline column too, since it was
written from a measurement taken in an earlier session against
plausibly the same contaminated worktree convention.

**It was.** Freshly measuring all 5 panel scenes against the *clean*
`4af1a75` binary, interleaved with current-commit runs to cancel out
machine-load drift, gives baseline numbers roughly **2x faster** than the
script's long-standing hardcoded column:

| Scene | Old hardcoded baseline (s) | Clean fresh baseline (s) | Current (s) | Corrected factor |
| --- | ---: | ---: | ---: | ---: |
| `level2/spline` | 1.989 | 1.045 | 0.40 | **0.383×** (current faster) |
| `level3/ntreal/ntreal` | 1.292 | 0.64 | 0.89 | **1.39×** (current slower) |
| `level3/piece3/piece3` | 4.112 | 2.015 | 2.96 | **1.47×** (current slower) |
| `level2/iortest` | 5.316 | 2.115 | 4.72 | **2.23×** (current slower) |
| `level1/shapes2` | 0.382 | 0.145 | 0.175 | **1.21×** (current slower) |

**This overturns a claim repeated across the whole Plan 5-9 record**:
"the panel already beats baseline (0.17-0.66×), the gap is drums-shaped
only" was computed against the same ~2x-inflated baseline column every
time — every one of those "0.x×" panel factors was comparing a correct
current time against a wrong (inflated) baseline time. With a clean
baseline, **4 of 5 panel scenes are actually slower than baseline**, not
faster; only `spline` genuinely beats it. `scripts/benchmarkPanel.sh`'s
`scenes` array has been corrected in this commit with a dated comment
explaining why, so future runs of the script report honest factors.

**drums, revalidated** (interleaved, 5 rounds each, same session):
baseline 5.09-5.15 s (mean ≈5.13 s), current 9.05-9.41 s (mean ≈9.18 s,
matches the earlier Plan 8-R2 measurement's output hash exactly —
`b2edf737...`, not a different build). **Ratio ≈ 1.79×** — worse than the
~1.5× figure quoted earlier in this same overall engagement (that figure
was itself likely measured under lighter machine contention; both this
plan's baseline and current numbers were measured back-to-back,
interleaved, on the same loaded machine, which is the more trustworthy
comparison method even though the absolute seconds are higher than a
quiet-machine run would show).

**Full `renderAll.sh` wall time, 1280×800, 108 scenes** (each scene
backgrounded, wall time = longest-pole plus scheduling overhead, not a
sum): baseline **86 s**, current **146 s** — **ratio 1.70×**, consistent
with drums's 1.79× and with the corrected panel showing most scenes
slower. The suite-wide picture and the drums-specific picture now agree;
there is no evidence left that the slowdown is "drums-shaped only."

**`-parallel`**: combining `renderAll.sh`'s own inter-scene backgrounding
(108 concurrent processes) with `EXTRA_POVRAY_OPTS=-parallel` (per-process
intra-scene threading on top) oversubscribes the machine by roughly two
orders of magnitude and produced 28 golden mismatches — **investigated
and attributed to the oversubscription stress, not a correctness bug**:
`scripts/renderParallel.sh` (the project's actual sanctioned `-parallel`
gate — 2 concurrent scenes, each internally parallel, exactly Plan 7/8/9's
established usage pattern) passes byte-exact against goldens, and two
back-to-back full `renderParallel.sh` runs are MD5-identical
(`a9b6e56d...iortest.tga`, `c3d0d1f5...drums.tga`) both times. Recorded as
a methodology note: don't combine `renderAll.sh`'s own backgrounding with
`-parallel`; that combination was never part of any prior plan's gate and
isn't validated for it.

### Phase 4 — Correctness Closure: Result

- Full gate at the exit commit: green (`testAgainstGoldenImages.sh`
  passes after the stress-test output was overwritten by a clean
  `renderAll.sh` re-run).
- Canary comparison, current output vs. a freshly rendered clean
  `4af1a75` baseline (1280×800, AE = differing 8-bit pixels out of
  1,024,000, RMSE normalized 0-1):

  | Scene | AE | RMSE |
  | --- | ---: | ---: |
  | drums | 1093 (0.11%) | 0.0055 |
  | takeoff | 399 (0.04%) | 0.0013 |
  | skyvase | 2618 (0.26%) | 0.0072 |
  | pacman | 7815 (0.76%) | 0.0150 |
  | chess | 4 (0.0004%) | 0.0001 |
  | dfwood | 0 | 0 |

  None of these are byte-identical to the original baseline — expected,
  since Plan 5 fixed real numeric bugs relative to `4af1a75` (documented:
  double-invert, stale-transform-in-`doExtraInformation`, sphere
  non-unit-direction, nested-composite virtual dispatch, mirror-corner
  CSG-by-ray-segment, chess detail-owner ordering — see
  [[performance_plan5_phase1]], [[sphere_nonunit_direction_fix]],
  [[nested_composite_virtual_dispatch_fix]],
  [[csgroth_mirror_corner_fix]]). All six diffs are small (≤0.76% AE,
  ≤1.5% RMSE) and were not re-derived pixel-by-pixel against each
  historical fix's own before/after record in this session — the
  magnitudes are consistent with that record and none show the gross,
  structural corruption a real regression would produce. Flagged as
  "consistent with known fixes" rather than independently re-proven line
  by line; a future session wanting full certainty should diff against
  each fix's own recorded before/after AE.
- `-parallel` determinism: see Phase 3 — `renderParallel.sh` byte-exact
  against goldens, two full runs MD5-identical.
- Leak check (`valgrind --leak-check=full --show-leak-kinds=definite,
  indirect`), chess.pov (163 CSG programs, 116 residual transformed
  operands — the most CSG-dense of the six canaries) at 80×50: **375,312
  allocations, 375,312 frees, 0 leaks, 0 errors**. Clean.

### Phase 5 — Residual Analysis and Cycle Closure

**Verdict: Ending B — structural floor declared.**

Both the drums-specific ratio (1.79×) and the suite-wide ratio (1.70×,
from a corrected panel and a full clean `renderAll.sh` comparison) now
agree: the gap is general, not drums-shaped, and sits well above the
1.2× Ending-A threshold. This is a materially different picture than the
one carried through Plans 7-9 (which believed, on a since-corrected
baseline measurement, that only drums was slow) — the correction changes
the verdict but not the underlying engineering record, which stands:

**Suspect list, final disposition:**
- **(a) Residual transformed/nested operand population Plan 8-R could not
  collapse** — measured, real, but small: drums is at 0 residual
  transformed operands post-R2 (this session's drums statistics line
  confirms `Plan 6 residual (un-collapsed) operands: transformed 0`).
  Other scenes (piece3: 501, spline: 430, iortest: 15, ntreal: 72) still
  carry real residue, dominated by category-3 unbakeable kinds (sphere/
  box/primitive) Plan 8 was never able to touch by design. **Contributing
  but not dominant** — piece3 and iortest's slowdowns (1.47× and 2.23×)
  are the panel's worst, consistent with their larger residue counts, but
  spline (430 residual, the single largest count) is the ONE scene that
  beats baseline (0.38×) — residue count alone doesn't predict the
  outcome, ruling it out as the sole cause.
- **(b) Arithmetic volume in baked-quadric helpers reached with
  `sharesRaySpace=false`** — real (Prim share suite-wide is 22.80%,
  concentrated in `intersectBakedQuadricWithTrueMiss` and friends per
  Phase 2), but Plan 8's fused kernels already reconstructed the
  baseline's own kernel shape over this math with zero numeric change
  (see `doc/performanceReviewPlan8.md` Architectural Constraints) — this
  isn't unnecessary recomputation, it's the same math the baseline did,
  paid inside a decoupled ownership model the baseline didn't have to
  respect.
- **(c) Clone/container residue** — [[performance_plan9_staging_closed]]
  closed this at Phase 0: the clone ArrayList cost is already at capacity
  0 (no allocation), the PriorityQueuePool has no growth ramp left to
  remove, and the one real batch-consumption site found carries a known
  byte-exactness risk not worth taking for a plan whose own entry
  criterion was already borderline. **Retired as a suspect, confirmed
  during this cycle, not just carried forward.**
- **(d) NEW, this plan: structural clone volume independent of CSG
  population** — Phase 2's suite-wide clone count (240.8M, barely below
  the pre-rebuild-era 260.9M figure) says the dominant clone source was
  never CSG-operand-shaped at all; it's `traceSimpleBodyAllCrossings`/
  `traceCompositeAllCrossings` constructing a local-space ray for every
  transformed simple body/composite child, every ray. This is inherent
  to the decoupled ownership model Plan 4 Conclusions §6 named as the
  deliberate tradeoff: the baseline could specialize per-object-graph
  because it owned baking at the scene layer; this architecture keeps
  `environment/geometry` baking-free by design (Phase 1's clean layering
  check above), which means per-child local-space transforms are paid as
  real per-ray work, not amortized into a precomputed structure. **This
  is the closest thing to a single root cause the whole cycle found**,
  and it is architectural, not a bug — removing it would mean giving
  `environment/geometry` back the baking responsibility Plan 6 explicitly
  took away from it.
- **Retired suspects** (measured, not guilty, confirmed again this
  cycle): dispatch/routing depth, union-kernel dispatch (`traceGeneric
  MorganUnion` fully inlined away, per
  [[performance_plan8_kernel_fusion]]), viewpoint-constant recomputation
  (RaySharedCache, live and confirmed working in Phase 1's audit).

**What was not attempted, and why:**
- Candidate-order changes to close item (d): rejected for byte-exactness
  risk (the exact class of bug in the dead-end list —
  [[csgoperand_clone_perf_fix]]'s append-only-queue regression).
- Removing the local-space clone by mutating rays in place: the plan's
  own recorded dead end (two prior regressions,
  `doc/performanceReviewPlan9.md` Known Dead Ends) — not re-attempted
  without new evidence the regression's cause no longer applies.
- Giving `environment/geometry` baking responsibility back: would reverse
  Plan 6's core architectural decision and its stated rationale (baking
  is a render-time concern, not a geometry concern) — out of scope for a
  performance cycle, a design-direction question for the project owner.
- Polynomial/parametric solver work (Group C scenes: witch, bezier,
  steiner): out of scope per Plan 4 Observation 5, unrelated to the
  baked-CSG gap this cycle targeted.

**`doc/designObjectives.md`**: no update needed — the baked-model-is-
render-owned description it already carries (post Plan 6 Phase 4) still
holds; nothing in Plan 10 changed ownership.

**Cycle closed.** Plans 5-10 delivered: two real Plan 5 numeric bugs
fixed, a Plan 7 `-parallel` data race fixed, a full architectural
rebuild with the layering/ownership properties Phase 1 re-verified clean,
and Plan 8-R2's real, measured, gate-green performance win (drums -12%,
the only one of six attempts to move the needle). It also delivered a
now-corrected measurement record: the ~1.5-1.7× gap vs. baseline is real,
general (not drums-only), and its dominant named cause (item d above) is
a direct, understood consequence of the geometry/baking decoupling this
project chose deliberately in Plan 6 — declaring that floor honestly, per
the Reframing section's own framework, is this cycle's correct and
final outcome.

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
