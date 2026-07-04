# Performance Review Plan 17: The Last Named Levers — and the Honest Parity Question

## Scope Decision (user, 2026-07-04) — read this before executing

The product decision §1 asks for has been made, and it re-scopes this plan:

1. **Thread safety is non-negotiable**: `-parallel` stays. No phase may
   reintroduce shared mutable state on scene/baked objects (the Plan 7
   class of bug). This rules out un-buying purchase (1) permanently.
2. **This final optimization push focuses on shadows vs composites**:
   Phase 2 is the centerpiece.
3. **The identity-fold transform skip (Phase 1) is judged cheap
   gain-for-risk**: execute it.

Therefore: **execute Phase 0 (censuses 1 and 3 only), Phase 1, Phase 2,
and Phase 5**. Phase 3 (composite collapse) and Phase 4 (second compiled
pattern) are **deferred indefinitely** — do not start them under this
plan; census 2 is optional and only worth running if it costs nothing
extra. Phase 5's Ending-A threshold applies to the reduced scope: any
measurable pole improvement with a green gate closes this plan
successfully; the terminal-floor statement covers whatever remains.

## Position in the Plan Sequence

Opens a new (short) cycle after Plan 16 closed the 11–16 cycle on Ending B.
It exists to answer one user question with evidence: **is the remaining
116 s vs 85 s gate gap recoverable, or is this the end of the road — and if
so, why?** It contains every remaining lever that has a *name and a design*
(not just a profile bucket), ranked by risk, plus the explicit statement of
what full parity would actually cost. If all phases here land or are
measured out, there is no Plan 18 backlog left — only architecture changes.

## §1 — Why 85 s parity is not reachable by increments (recorded up front)

Plan 11 §1 established the decisive fact: at drums 320×200 the current tree
executes **0.51× the primitive-test work** of the baseline (81.4 M vs
160.9 M tests) but at **~1/3 the throughput** (10.8 M/s vs 31 M/s). If the
current work volume ran at baseline per-test cost, drums would take ~2.6 s
— *faster* than the baseline's 5.14 s. So parity is not blocked by
arithmetic; it is blocked by the orchestration wrapped around each test.
That orchestration is not waste — each layer was bought deliberately, and
each is only removable by giving its feature back:

1. **`-parallel` determinism** (Plan 7 / [[parallel_raytracer_plan]]): the
   baseline caches viewpoint constants in *mutable fields on shared shape
   objects* — free to read, and a data race under any concurrency. The
   current tree pays an indexed `RaySharedCache` lookup per test instead.
   The baseline binary cannot run `-parallel` correctly at all.
2. **Layering** (`environment/geometry` transform-free): where baking
   cannot reach (unbakeable primitives, composites), transforms happen via
   per-ray matrix work and `LocalIntersectionClone`s — 27.3 M/render in
   drums. The baseline let shapes own their transforms and mutated rays in
   place (an established byte-exactness dead end here, see
   [[csgoperand_clone_perf_fix]]).
3. **The deterministic candidate model**: every crossing becomes a 176-byte
   `IntersectionCandidate` in a binary heap, carrying the annotation
   machinery (detail owners, per-operand materials) that the texture
   correctness fixes ([[texture_frame_fixes_part7]]) depend on. The
   baseline pushed raw depth doubles into flat arrays.
4. **Morgan interpreter generality**: correct nested-CSG semantics for the
   cases the baseline got wrong or never exercised.

The recoverable slice is what is left *inside* these constraints. The four
phases below are that slice, with expected magnitudes; realistic best case
if everything lands is drums ~6.0–6.5 s (vs 5.14 baseline) and renderAll
~100–108 s (vs 85). Beyond that: Ending B becomes terminal, and the honest
answer to "why not 85 s" is the four purchases above.

## §2 — Evidence inventory (measured 2026-07-04, Plan 16 exit state)

- drums 320×200: 7.55 s vs 5.14 s (1.47×); iortest 3.38 s vs 2.10 s
  (1.61×); renderAll 116 s vs 85 s (1.365×). Poles unchanged (drums
  +124 s offset, iortest +70 s; the rest of the suite ≤55 s).
- **Shadow load is enormous and partially unoptimized**: drums performs
  6,506,794 shadow-ray object tests for 141,823 rays (46 per ray); iortest
  13,429,975 (35 per ray). The shadow path already has AABB pre-cull,
  first-blocker early exit, and a CSG first-hit fast path — but
  `canUseCsgFirstHitForShadow` (`DirectLightShader.cpp:57-62`) **excludes
  `TraceKind::Composite`**, so shadow rays against drums' 13 composites pay
  the full clone + all-crossings + queue-drain path every time.
- **The hottest kernel recomputes provably-identity transforms**:
  `traceTransformedNestedSingleCorePlaneOperandAllCrossings`
  (`BakedCsgTrace.cpp:1443`, 42.5 M calls/drums) always executes
  `localToObject.transformPoint/transformDirection` on the parent ray
  (lines 1469–1481) even when `parentOperand.pushdownFolded` — whose own
  Plan 8 R2 comment (line 1494) asserts the matrices are identity and the
  results "bit-equal to the parent ray's own origin/direction".
- **The unspecialized intersection residue**:
  `traceMorganIntersectionGeneric` — 3.0 M calls, 7.3% self (non-LTO
  profile). drums compiles 250/450 CSG programs to the core-plane plan;
  the other 200 stay generic-morgan. No census exists yet of what operand
  signatures those 200 have.
- **Composite clone volume**: 12,502,766
  `traceCompositeAllCrossingsInCompositeSpace` calls/drums render —
  reconfirmed exactly at Plan 16 exit. Plan 15's all-or-nothing collapse
  design is complete (ordering proof included) and blocked only on two
  specified correctness fixes plus a test scene that exercises them
  (`doc/performanceReviewPlan15.md` Phase 2 Results).
- **Measurement caveat (binding on all phases)**: every gprof table in
  Plans 11–17 comes from a non-LTO `-pg` build; the shipping binary is
  LTO. Trivial out-of-line entries (capacity-0 `ArrayList` ctor/dtor pairs,
  `PriorityQueuePool::pop`) are partly profiling artifacts — the clone
  constructor is already header-inline with no heap allocation
  (`RayWithSegments.h:79-104`), and `pop` is an intrusive free-list
  (`PriorityQueuePool.txx:53-71`). **Do not spend a phase on them.**
- **Dispatch mechanics are a closed question** (analyzed 2026-07-04 on user
  request): the baked layer already dispatches by enum switch + direct
  static calls (`CsgOperandKind`/`TraceKind`), which strictly dominates
  function-pointer/manual-vtable schemes (indirect calls can't inline).
  Only 8 virtual `geometry->` call sites remain, all on cold/unbakeable
  paths; in drums the census shows 0 unbakeable operands. Ceiling <1%.
  Do not reopen.

## Architectural Constraints (unchanged)

All changes live in `render/bakedScene` and `render/shaders`;
`environment/geometry` stays transform-free; `environment/scene` stays
parsed data + `TransformStep` lists. Byte-identical gate required for
Phases 1–2; Phase 3 re-enters the Golden-Image Evaluation Protocol with a
new test scene added *first*.

## Phase 0 — Three Censuses (evidence, no shipped code)

Temporary counters (the [[performance_plan14_closed_no_code]] pattern —
atomic, printed at exit, fully reverted before close):

1. **Shadow/composite share**: of the 12.5 M composite-space traces and
   14.9 M `traceCompositeAllCrossings` calls in drums, how many carry
   `isShadowRay`? Decides whether Phase 2 (shadow fast path) or Phase 3
   (full collapse) owns the composite volume.
2. **Generic-morgan signatures**: for each of drums'/iortest's
   generic-morgan programs, record the operand-kind multiset and operation.
   If one signature covers ≥40% of `traceMorganIntersectionGeneric`
   traffic, Phase 4 has a target; otherwise Phase 4 is skipped with the
   histogram published.
3. **Identity-fold share**: what fraction of the 42.5 M core-plane kernel
   calls have `pushdownFolded` set. Sizes Phase 1.

Exit: ranked table of expected payoffs. Any phase whose measured share is
under ~5% of drums self-time is skipped, with the number recorded.

## Phase 1 — Identity-Fold Transform Skip (lowest risk, smallest win)

Where `pushdownFolded` is set, bypass the four `transformPoint`/
`transformDirection` calls in
`traceTransformedNestedSingleCorePlaneOperandAllCrossings` and alias the
parent ray's origin/direction directly (Plan 8 R2 already asserts
bit-equality; note `const Vector3Dd&` aliasing, not copies). Guard against
the one known IEEE hazard: an identity matrix multiply can flip `-0.0` to
`+0.0`, so before shipping, run a debug assert mode that computes both
paths and memcmp's them across full drums + chess renders (the Plan 14
side-by-side pattern). Gate must stay byte-identical; if the assert ever
fires, record it and close the phase — do not "fix" it with tolerance.

## Phase 2 — Shadow First-Hit for Composites

Extend the existing shadow fast path (`DirectLightShader.cpp:75-89`) to
`TraceKind::Composite`: add a composite branch to `BakedTrace::traceFirstHit`
that clones into composite space once (reusing the
`traceCompositeAllCrossingsInCompositeSpace` preamble: bounding shapes
tested in composite-local space, clipping via `compositeLocalPoint`),
first-hits each child through the existing per-kind first-hit machinery,
takes the min-t, transforms the winner back. **Correctness shape is already
established by the existing non-composite fast path**: it only
short-circuits to `true` when the first hit lands strictly inside the
shadow window with `noShadowFlag` clear; every other outcome falls through
to the unchanged all-crossings path. Since the all-crossings drain
(`traceShadowObject`, `DirectLightShader.cpp:94-115`) returns `true` on the
first in-window non-noShadow candidate *in t-order*, and first-hit produces
exactly the min-t candidate, the boolean is provably identical whenever the
fast path fires. Nested composites recurse. Gate must stay byte-identical
(the light attenuation result is the same boolean/color either way).

Risk note: the composite children's `noShadowFlag` dead-field finding from
Plan 15 does **not** bite here — the fast path falls through unless the
flag is clear, and the fallback path is today's behavior verbatim.

## Phase 3 — Composite Collapse (Plan 15 iteration 2, executed this time)

Entry criterion: Phase 0's census still attributes ≥10% of drums to
composite-space tracing *after* Phase 2 lands (shadow rays may own more of
it than expected — measure, don't assume).

1. **Author the missing test scene first**: nested composites (≥2 levels)
   with quadric/plane leaves, mixed per-child `no_shadow` flags across
   nesting levels, and a bounded transformed composite — the exact cases
   Plan 15 found unexercised. Render it through the *current* path, get
   user confirmation, and add it to the golden set **before** any refactor.
2. Implement the two documented correctness fixes: (a) recompute a child's
   world AABB through the accumulated enclosing-composite step chain
   (never reuse the existing composite-local `worldBounds`); (b) propagate
   `noShadowFlag` as an OR down the enclosing chain onto flattened
   children.
3. Implement the all-or-nothing inlining exactly per Plan 15's design
   (collapse only when the entire subtree is bounding/clipping-free with
   Quadric/InfinitePlane leaves; forward-insert flattened children at the
   composite's original list position — the ordering proof is already
   written in `doc/performanceReviewPlan15.md`).
4. Full Golden-Image Evaluation Protocol ([[performance_plans_5_to_10]]),
   kill-switch kept until the gate has been green through two full runs.

Expected: removes most of drums' 12.5 M clones and both composite dispatch
frames — the single largest named item left (~8–12% of drums).

## Phase 4 — Second Compiled CSG Pattern (conditional on Phase 0)

If the signature census names a dominant generic-morgan shape, compile it
the way `SingleCorePlaneIntersection` was built (Plans 6/8 methodology: new
`CsgPlanKind`, build-time classifier in `BakedSceneBuilder`, fused
trace-time kernel, byte-identical gate). If the census is flat, skip and
publish the histogram — that residue is then interpreter generality by
definition and belongs to §1's item 4.

## Phase 5 — Close: Ending A, or the Terminal Floor Statement

Plan 16 measurement methodology verbatim (interleaved, same session, pole
analysis, gprof before/after). Two endings:

- **Ending A**: renderAll ≤ 105 s (revised from Plan 16's 100 s — §1's
  arithmetic caps perfect execution around drums 6.0–6.5 s, and drums *is*
  the wall time).
- **Ending B (terminal)**: if the phases land green but the poles don't
  move enough, write the final floor statement: what remains is the
  candidate-queue model, interpreter generality, and unbakeable-primitive
  clones — i.e., the architecture itself. Parity with `4af1a75` is then
  formally out of scope for *optimization* plans: it requires un-buying
  `-parallel` safety, layering, or byte-exact determinism (§1), which is a
  product decision, not a performance review. No Plan 18 should be opened
  against the same targets without first making that product decision.

## Acceptance Criteria

- Phase 0 histograms published here; every later phase cites its measured
  share as entry evidence, and skipped phases record the disqualifying
  number.
- Phases 1–2: gate byte-identical, zero re-baselines, kill-switch removed
  only after two green runs.
- Phase 3: new golden scene added and confirmed *before* the refactor;
  protocol table for any flagged scene.
- Phase 5 table: drums/iortest/panel/renderAll vs both Plan 16 exit and
  baseline, plus final gprof top-10 vs Plan 11's.
- Either ending explicitly declared; if Ending B, the §1 purchase list is
  the published answer to "why not 85 s".

## Results (2026-07-04)

Executed per the Scope Decision: Phase 0 censuses 1 and 3, Phase 1, Phase 2
(attempted, closed without shipping code - see below), Phase 5. Census 2 and
Phases 3-4 were not started, per scope.

### Phase 0 census results

- **Census 3 (identity-fold share)**: instrumented
  `traceTransformedNestedSingleCorePlaneOperandAllCrossings` with atomic
  counters, ran drums (320x200) and iortest (320x200), reverted before
  close. Result: **100.00% of core-plane kernel calls have
  `pushdownFolded` set** in both scenes (drums 42,515,052/42,515,052;
  iortest 10,786,387/10,786,387). Every single call to this kernel was
  paying a fully-avoidable no-op transform. Sized Phase 1 at its maximum
  possible payoff for this kernel.
- **Census 1 (shadow/composite share)**: instrumented
  `traceCompositeAllCrossingsInCompositeSpace`, ran drums only (iortest has
  zero composites). Result: **87.02% of composite-space traces carry
  `isShadowRay`** (7,546,970/8,673,143). Confirms shadow rays dominate
  composite traffic in drums, as §2 assumed - real evidence for Phase 2's
  entry criterion, not just intuition.
- Both counters were temporary (`std::atomic` + destructor-printed
  totals), added and fully reverted in the same session; `git diff --stat`
  on the two touched files was empty before Phase 1 began.

### Phase 1 - shipped

Implemented exactly as scoped: in
`traceTransformedNestedSingleCorePlaneOperandAllCrossings`
(`BakedCsgTrace.cpp`), when `parentOperand.pushdownFolded` is set, the
parent ray's origin/direction are aliased by pointer instead of paying
`localToObject.transformPoint`/`transformDirection`. Verification: a
`PLAN17_PHASE1_ASSERT_MODE` build (temporary `target_compile_definitions`,
reverted before shipping) computed both the aliased and the transformed
values under the folded branch and `memcmp`'d them; ran drums, chess, and
the full 108-scene `renderAll.sh` suite - **zero assertion fires**. Shipped
build (no assert overhead) passed the full golden-image gate byte-identical
(`Test passed.`).

### Phase 2 - attempted, closed without shipping code

Implemented per the plan: `BakedTrace::traceCompositeFirstHit` gained a
genuine per-child-first-hit fast path (`traceCompositeFirstHitDirect`,
merge over `BakedTrace::traceFirstHit` per child instead of full
`traceCompositeAllCrossings` enumeration) for composites without clipping
shapes, and `canUseCsgFirstHitForShadow` was extended to composites. This
compiled and ran, but **failed the golden-image gate**: drums, fish13,
ionic5, snack, wg5 differed (drums AE=243730); total suite time dropped
114s -> 80s, confirming the mechanism did save real work, but not
correctly.

**Root cause, isolated empirically**: a debug-compare build
(`PLAN17_PHASE2_DEBUG_COMPARE`, temporary) logging every case where the
fast path's winning t disagreed with the reference all-crossings-derived
t showed the fast path always returning a **farther** (never nearer) hit
- e.g. `refT=87.024` vs `directT=87.152` in drums. Swapping the per-child
query from `BakedTrace::traceFirstHit` to `BakedTrace::traceAllCrossings`
+ `peek()` (keeping every other line identical: same children, same AABB
cull, same merge-and-transform-back logic) **eliminated every mismatch**.
This isolates the defect to `BakedCsgTrace`'s compiled first-hit path for
`CsgPlanKind::SingleCorePlaneIntersection` (drums is 250/450 core-plane
programs) - it does not always return the CSG program's true nearest
boundary crossing. The bug is pre-existing and latent, not introduced by
this plan: every prior consumer of `BakedTrace::traceFirstHit` on a
CSG-kind object only needed a boolean answer (bounding-shape pass/fail, or
"is *some* qualifying hit in the shadow window") drawn from a single
object, never a numeric minimum compared across several sibling objects
the way a composite's per-child merge requires. That comparison is new in
this codebase, and it is what exposed the gap.

Fixing `BakedCsgTrace`'s compiled single-core-plane first-hit algorithm is
a nontrivial, unscoped change to code with ~10+ call sites and its own
byte-exactness gate - explicitly out of this plan's reduced scope ("no
phase may reintroduce..." / centerpiece-but-conservative framing). Per
this plan's own standing instruction for Phase 1 ("if the assert ever
fires... close the phase - do not fix it with tolerance"), the same
discipline applies here: **all Phase 2 code was reverted** (`git checkout`
on `BakedTrace.{h,cpp}` and `DirectLightShader.cpp`); only the Phase 0
census evidence and this finding survive, for whichever plan takes up
CSG first-hit correctness next. Flagged as the **named entry point for a
future Plan 18**, not attempted further here.

### Phase 5 - measurement and close

Interleaved, same session, Phase-1-only shipped build (LTO, no debug/assert
defines), golden gate green throughout:

| Scene | Plan 16 exit | This session | Baseline (4af1a75) |
|---|---|---|---|
| drums 320x200 | 7.55 s | 7.22 s (avg of 3) | 5.14 s |
| iortest 320x200 | 3.38 s | 3.28 s (avg of 3) | 2.10 s |
| renderAll.sh (full suite) | 116 s | 112 s | 85 s |

drums improved ~4.4%, iortest ~3.0%, renderAll ~3.4% - small but real,
consistent with Phase 1's actual reach (one kernel, 100% of its calls
benefit, but that kernel is one of several hot paths). No gprof
re-profiling was run for this close: Phase 1's mechanism is fully
characterized by direct inspection and the Phase 0 census (100% fold
share, a fixed four-call-elimination per hit), and Phase 2 shipped no
code, so there is nothing new for a profile to attribute.

**Ending: B (terminal), with one net phase shipped.** renderAll at 112 s
does not reach the revised Ending-A bar (<=105 s) - not because Phase 1
underperformed its ceiling (it hit 100% of its addressable calls), but
because Phase 2, the phase actually sized to move the pole scenes
(drums/iortest), could not ship safely. The gap this plan closes with is
therefore mostly the same one Plan 16 closed with, less a few percent: the
architecture-level purchases in §1 (parallel-safety, layering, the
candidate-queue model, interpreter generality) remain the wall, *plus* one
newly identified concrete defect (CSG compiled-first-hit non-minimality)
that blocks the one clear remaining win this plan could name. No Plan 18
should reopen the composite-shadow-fast-path target without first fixing
that defect (or explicitly re-scoping around it, e.g. restricting any
future attempt to composites whose children are all direct primitives,
never CSG).

## Known Dead Ends (cumulative; do not repeat)

- Function-pointer / manual-vtable dispatch in the baked layer (§2 above).
- Heap consumption-order changes and drain batching
  ([[performance_plan9_staging_closed]]).
- Append-only scratch queues; in-place ray mutation
  ([[csgoperand_clone_perf_fix]]).
- Naive `detailOwners` shrinking or spill-at-extraction
  ([[performance_plan14_closed_no_code]] — in-queue mutation).
- Indirect candidate heap without the side-by-side equivalence harness
  (137 call sites; same memory).
- Partial per-child composite baking (Plan 15 iteration 1 — shared clone
  makes it a no-op with extra risk).
- Sphere→TransformedQuadric conversion ([[phase_h_bakedscene_refactor]] —
  numerically different, 35 gate failures).
- Composing replay steps into one matrix (Plan 5 — not bit-identical).
- Trusting absolute timings across sessions or from worktree builds
  without interleaving ([[performance_plan13_phase1_methodology_lesson]],
  Plan 10's `-pg` contamination).
- Composite first-hit via per-child `BakedTrace::traceFirstHit` merge
  (Plan 17 Phase 2) - correct in design (proven equivalent for composites
  without clipping shapes) but exposes a real, pre-existing
  non-minimality bug in `BakedCsgTrace`'s compiled
  `SingleCorePlaneIntersection` first-hit path, never before compared
  numerically against a sibling object's hit. Do not retry without first
  fixing (or scoping around) that CSG-level defect.
