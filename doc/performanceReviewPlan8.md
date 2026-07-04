# Performance Review Plan 8: Fused CSG Execution Kernels — Collapsing the Interpreter

## Position in the Plan Sequence

Fourth of six (Plans 5–10; sequence table in `doc/performanceReviewPlan5.md`).
Requires Plans 5–7: world-space baked operands, the rebuilt
`render/bakedScene` model with per-plan `BakedCsgProgram` tables, and the
per-ray shared cache. This plan is Plan 4 §6.3 ("collapse the interpreter
layers") executed on the new foundation.

## Motivation (from Plan 4, Conclusions §3)

Baseline call path per (ray × CSG object): scene loop → 1 virtual call →
`allCsgIntersectIntersections` (a 50-line two-nested-loop kernel) → 1 virtual
call per operand → primitive math. Three functions held 62% of all time.

The pre-rebuild path was: `RenderEngine::trace` →
`traceObjectAllCrossings` → `traceAllCrossings` → `traceAllCrossingsWithScratch`
→ plan dispatch → `traceOperandAllCrossings` (per-operand kind switch + AABB
+ material resolution) → `intersectBaked*` — the same work smeared over 93
functions, none above 7.4%, with 5–7 arguments shuffled per level. Plan 6
removed the per-ray metadata reads and material resolution but deliberately
ported the traversal shape. This plan removes the traversal shape itself:
**one fused loop per plan kind, chosen once at build time.**

After Plans 5–7 the operands a kernel iterates are world-space geometries in
flat arrays with a shared per-ray cache — exactly the conditions under which
the baseline kernel was fast. The fused kernels are therefore a
reconstruction of the baseline's Morgan kernel shape over the new data
model, per plan kind, with zero numeric change.

## Architectural Constraints

- All work happens inside `render/bakedScene` (kernels) and consumes
  `render/raySharedCache`. `environment/geometry` and `environment/scene`
  are untouched.
- Kernels receive `(const BakedCsgProgram&, const RayWithSegments&,
  RaySharedCache&, CsgScratchContext&, results)` — nothing else. If a kernel
  needs more, the builder failed to bake something; fix the builder, not the
  signature.
- Candidate ordering and queue semantics stay exactly as ported in Plan 6
  (byte-exactness). Queue *replacement* is Plan 9, not here.

## Design

One kernel function per `BakedCsgPlanKind` × {allCrossings, firstHit,
shadow}. The build-time plan choice becomes a stored function pointer (or a
single top-level switch in `BakedTrace` — measure both; the switch is
usually faster than an indirect call when the kind population is small, and
it inlines):

```
traceCsgAllCrossings(program, ray, cache, scratch, out)
    → kernelUnionAllCrossings(...)            // fused: loop operands inline,
    → kernelIntersectionAllCrossings(...)     // intersect primitive inline,
    → kernelSingleCorePlaneAllCrossings(...)  // membership merge inline
    → kernelTopLevelPlaneUnionAllCrossings(...)
    → kernelDisjointBoundedUnionAllCrossings(...)
    → kernelRaySegments(...)                  // -csgRoth algorithm scenes
    → kernelFallback(...)                     // anything not yet fused
```

Inside a fused kernel:

- the operand loop iterates the flat SoA arrays directly
  (`operandGeometry[]`, plane arrays, bounds) — no per-operand function call
  for classification, no AABB helper call (inline the slab test), no
  per-operand kind switch: the builder already partitioned operand index
  ranges by kind (planes first, direct quadrics next, residual transformed
  last — the partition order is a builder guarantee documented in
  `BakedCsgProgram.h`);
- primitive intersections call the geometry routine directly
  (`Quadric::doIntersectionForAllRayCrossings`-equivalent via the cache-aware
  baked helper for quadrics; direct calls for plane/sphere) — one call level
  above the math, as in the baseline;
- containment/membership merging (`mergeByMembership`,
  `containmentTestOperand` logic) is inlined into the kernel body where the
  profile justifies it, keeping the exact test order;
- nested CSG children recurse through the same top-level kernel dispatch
  (depth is bounded by scene nesting; the baseline recursed too).

`static` + internal linkage (or members of a kernel class) so the compiler
can inline aggressively within the translation unit; keep each kernel in one
`.cpp` to give the optimizer the whole loop.

## Original Phases (1-6) — SUPERSEDED 2026-07-04

The original phase list (union kernel → intersection kernels → first-hit/
shadow kernels → remaining plan kinds → simple-body tightening → closing
measurement) was ordered by the Plan 4 heat table, which predates Plans 5-7
and proved stale on re-measurement. Phase 1 was executed (see Status below)
and confirmed the diagnosis that retired the rest: dispatch-level fusion has
no measurable headroom left on this tree. The compiler already inlines the
union kernel; four consecutive gate-green fusion/caching changes (Plan 7
Phases 2-4 plus Plan 8 Phase 1) each targeted profile-confirmed hot code and
none moved drums. Phases 2-6 as originally written are retired — their text
is in git history at commit f3ac202 and earlier. The refocused continuation
below replaces them.

## Measurement Gate

Every phase:

```bash
./scripts/clean.sh
./scripts/compile.sh
./scripts/renderAll.sh
./scripts/testAgainstGoldenImages.sh
```

Byte-identical output vs the Plan 7 exit state at every phase — fusion
re-arranges control flow, never arithmetic or ordering. Plus, per phase:
drums timing (3 runs) and a gprof run confirming the targeted functions
disappeared from the profile (their time must reappear inside the kernel,
not vanish into new helpers).

`-parallel` determinism check (two renders byte-compared) at Phases 3 and 6.

## Acceptance Criteria

- drums wall-clock: this is the plan expected to close most of the remaining
  gap. Target: ≤ 1.2× the baseline 5.10 s (≤ ~6.1 s) at Phase 6, measured on
  the same machine as the Plan 4 numbers.
- gprof concentration: ≥ 50% of drums self-time in ≤ 10 functions, with the
  top entries being kernels and primitive math, not routing.
- `traceOperandAllCrossings` and its successors: 0 calls (function deleted).
- `LocalIntersectionClone` on drums: only residual transformed operands may
  still clone; count reported and justified against the Plan 6 builder
  statistics.
- Fallback-kernel population across the suite documented (≈ 0).

## Status: Phase 1 attempted, paused pending strategy decision

Before starting, re-profiled drums with gprof on the current (post-Plan-7)
tree rather than trusting the Plan 4 heat table, which predates Plans 5-7
and no longer reflects reality. Current top drums self-time (320x200,
non-parallel):

| self% | calls | function |
| ---: | ---: | --- |
| 12.46% | 50.1M | `intersectBakedQuadricWithTrueMiss` |
| 9.76% | 53.4M | `traceOperandAllCrossings` |
| 8.59% | 56.8M | `traceMorganCsg` |
| 6.06% | 25.9M | `traceSimpleBodyAllCrossings` lambda |
| 6.06% | 23.5M | `traceTransformedNestedSingleCorePlaneOperandAllCrossings` |
| 5.56% | 3.0M | `traceMorganIntersectionGeneric` |
| 5.22% | 28.6M | `traceCompiledCoreOperandAllCrossings` |

Two findings changed the plan's Phase 1 target:
1. `traceGenericMorganUnion` does not appear in the profile at all - the
   compiler already fully inlines it into `traceMorganCsg` (both are
   `inline` header functions), so "fuse the union kernel" in the literal
   sense the plan describes is already done by the optimizer. There was no
   separate union-dispatch cost left to remove.
2. The `SingleCorePlaneIntersection`/core-plane family
   (`traceCompiledCoreOperandAllCrossings` +
   `traceTransformedNestedSingleCorePlaneOperandAllCrossings` + their
   first-hit counterparts) is now the dominant cluster in drums (250 of 450
   CSG programs use this specialization vs. 200 generic-Morgan), matching
   what Plan 7 already found. This is Phase 2's target, not Phase 1's.

Implemented anyway, scoped to what Phase 1 could still plausibly reach:
in `traceGenericMorganUnion`'s transformed-operand loop,
`TransformedQuadric` operands (the dominant residual-transformed kind) now
get a direct inlined fast path instead of calling
`traceOperandAllCrossings`'s generic kind-switch dispatch. Iteration order
was kept identical (same array, same direction) specifically to avoid
disturbing equal-depth candidate tie-breaking across operand kinds - a
historically real regression class in this codebase.

Gate: byte-identical (full suite), `-parallel4` determinism re-checked on
drums (AE=0), panel scenes unaffected.

**drums timing: no measurable change** (~8.7s before -> ~8.9s after,
noise-level). This is the **fourth consecutive fusion/caching attempt**
(Plan 7 Phases 2-4, and now this one) that is individually correct,
gate-green, and reasoned from a real profile finding, yet produces no
measurable drums improvement. The consistent pattern across all four
points to the same conclusion each time re-confirmed: drums's cost is
concentrated in `intersectBakedQuadricWithTrueMiss`'s raw arithmetic
(12-13% self-time, ~40 FLOPs x 50-68M calls) and in the sheer call volume
through the core-plane specialization, neither of which a dispatch-level
fusion touches - fusion removes call/switch overhead *around* the math, and
that overhead is evidently not where drums's remaining ~1.7x-vs-baseline
gap lives.

**Recommendation given to the user**: pause Plan 8's remaining phases
(2-6) rather than continue speculatively. Phase 2 (the core-plane kernel,
the plan's own next-highest-heat target and the one most likely by current
profile data to matter) is the reasonable next attempt *if* the user wants
to keep pushing on drums specifically, but the last four attempts targeting
correctly-identified hot functions all failed to move the number, so there
is no strong basis for confidence it will differ. The two directions with
actual remaining leverage per Plan 4's own Conclusions are (a) revisit
scene-owned coefficient baking with baseline-identical operation order
(Plan 4 Conclusions #1 / Plan 5's original, only partially realized idea)
or (b) accept the current ~1.7x-of-baseline drums figure as the practical
floor of this architecture and close the Plan 5-10 sequence's performance
chase, banking the real wins already delivered (correctness fixes, the
-parallel race fix, zero regressions across 108 scenes throughout).

## Refocused Continuation (2026-07-04): Collapse-Rate Expansion, Not Fusion

### Why the refocus

The evidence across Plans 5-8 converges on one diagnosis:

- drums performs 67.8M quadric tests per 142K rays (~478 tests/ray);
  50.1M of them go through `intersectBakedQuadricWithTrueMiss` at ~110+
  FLOPs each because the operand is in a **private local space**
  (`sharesRaySpace=false`), so neither the ray's aggregate cache nor the
  viewpoint-constant cache (both delivered by Plan 7) can help.
- The build statistics on drums read: **244 residual transformed operands
  vs only 150 collapsed quadrics, 0 collapsed planes.** The majority of the
  operand population never got the Plan 5 treatment.
- Every attempt to cheapen the *dispatch around* those tests failed to
  move wall-clock (4 documented attempts). The only change in the whole
  Plan 5-8 sequence that altered per-test arithmetic — Plan 5's collapse —
  is also the only mechanism with a proven per-test FLOP reduction
  (~110 → ~25), and it currently reaches a minority of drums's operands.

Conclusion: the remaining lever is **raising the collapse rate**, i.e.
getting more operands into world/ray-shared space at build time, not
rearranging the code that visits them. This is Plan 4 Conclusions §6.1
("the only direction that attacks §1, §2, and §4 at once") applied to the
population Plan 5 left behind.

### Where the uncollapsed population comes from (code-verified 2026-07-04)

`bakeCsgOperand` (BakedSceneBuilder.cpp) collapses an operand only when
**all three** hold: `hasTransform`, `getSteps().size() > 0`, and the
geometry is a `Quadric` or `InfinitePlane`. Therefore residual transformed
operands are exactly:

1. `TransformedNestedCsg` — a nested CSG under a parent-level transform.
   **Never collapsible today by construction**, and the drums profile shows
   this is where the heat is: 23.5M calls/frame into
   `traceTransformedNestedSingleCorePlaneOperandAllCrossings`, which
   re-transforms the ray into nested space and then (for transformed cores)
   again into core space, per ray per visit.
2. `TransformedQuadric` with an **empty step list** — the matrices were
   composed by some path that skipped `steps.add()` (the copy constructor
   preserves steps, and `CsgOperand::translate/rotate/scale` all record, so
   any such population indicates a parser/instantiation path that sets
   matrices directly; must be found by measurement, not assumed).
3. `TransformedSphere` / `TransformedPrimitive` (Box, etc.) — no
   coefficient congruence exists; only a different representation (e.g.
   sphere center/radius transform for rigid transforms) could collapse
   these.

### Phase R0 — Categorize the residual population (measurement only)

Extend `BakedScene::Statistics` and the end-of-build report to split
`residualTransformedOperands` by the three categories above (and, for
category 1, count how many of the *nested children* under each transformed
parent are themselves bakeable quadric/plane kinds). Run on drums, chess,
pacman, takeoff, desk, iortest and record the table here. Gate: byte-
identical (statistics only).

**Kill criterion (honest-abort rule):** if categories 1+2 together account
for less than half of the residual population *weighted by the profile's
call counts* on drums, this plan closes immediately with the residue
documented — do not proceed to R1/R2 on hope; the four failed attempts
above are the cautionary precedent.

### Phase R1 — Close step-recording gaps (category 2, if it exists)

For every `TransformedQuadric`/`TransformedPlane` operand whose step list
is empty but whose transform exists: find the parser/instantiation path
that composed matrices without recording steps and record them there.
This re-uses `BakedGeometryBaker` unchanged. Gate: byte-exactness expected
to *break* for affected scenes in the same way Plan 5's collapses did
(baked-vs-per-ray rounding); apply the Plan 5 golden evaluation protocol
(explicit before/after diffs, user confirmation for re-baselines) — never
silent acceptance.

### Phase R2 — Nested transform push-down (category 1)

At build time, when a `TransformedNestedCsg` parent's step list is
non-empty and **every** operand of the nested program is a bakeable kind
(quadric/plane, or recursively an all-bakeable nested), rebuild the nested
program's operands with `parentSteps + childSteps` concatenated (the same
elementary-replay congruence Plan 5 validated) and reclassify the parent as
plain `NestedCsg`. The nested program then evaluates in parent space with
no per-ray parent transform, and its cores become world-space quadrics
eligible for the ray-shared caches.

Cautions recorded up front:
- Material transforms: `CsgOperand::translate/rotate/scale` also transform
  the material; push-down must reproduce exactly the material state the
  per-ray path produces today, or texture-space regressions will appear
  (cf. the wtorus layered-texture and §17 detail-owner history).
- A nested program may be shared by several parents (`#declare` reuse);
  push-down must clone the program per parent, not mutate the shared one.
  Builder statistics must report the resulting program-count growth.
- Candidate `t`/point computation changes coordinate space → byte-exactness
  will break for affected scenes; Plan 5 golden protocol applies, with
  drums/chess/skyvase/takeoff/pacman explicitly heat-mapped.

### Phase R3 — Closing measurement and sequence decision

drums (3 runs), panel, gprof, `-parallel` determinism. Then a written
go/no-go for Plan 9 based on what the exit profile actually shows (see the
re-scoped entry criterion in `doc/performanceReviewPlan9.md`).

### Phase R0 — Result (2026-07-04, commit pending)

Implemented as pure statistics: `BakedScene::Statistics` gained
`residualCategory{1,2,3}*` fields, populated in `accumulateStatistics`
(`BakedSceneBuilder.cpp`) using `classifyOperandKind`'s existing output plus
a new `nestedProgramFullyBakeable` recursive check (is every operand of a
`TransformedNestedCsg`'s nested program itself a bakeable kind — direct/
transformed quadric or plane, or recursively an all-bakeable `NestedCsg`).
Reported per-scene by `PovRayApplication::printStatistics`. Gate:
byte-identical full suite, confirmed.

| Scene | Residual total | Cat 1 (nested-CSG) | ...pushdown-eligible | Cat 2 (empty steps) | Cat 3 (unbakeable) |
| --- | ---: | ---: | ---: | ---: | ---: |
| drums | 244 | 244 | 124 | 0 | 0 |
| chess | 127 | 11 | 11 | 0 | 116 |
| pacman | 7 | 0 | 0 | 0 | 7 |
| takeoff | 4 | 1 | 0 | 0 | 3 |
| desk | 9 | 0 | 0 | 0 | 9 |
| iortest | 20 | 10 | 5 | 0 | 10 |

Category 2 (empty step list) is empty on all six scenes — the parser gap
hypothesized in the write-up does not exist on this tree; Phase R1 has
nothing to do and is closed as **not needed**.

drums is the decisive case: **100% of its residual population is category
1**, and 124/244 (51%) are push-down eligible *today*, with only a
one-level nested check — recursion wasn't even needed to clear half the
population. This lines up exactly with the profile: the 23.5M calls/frame
into `traceTransformedNestedSingleCorePlaneOperandAllCrossings` (Plan 8
Phase 1 status table) are category-1 territory by construction, since that
function only exists to serve `TransformedNestedCsg` operands.

**Kill criterion: does not fire.** Category 1 is not merely "the
call-weighted majority" of drums's residual population — it is all of it.
chess and iortest also show category 1 as the (smaller-scene) hot
candidate; pacman/takeoff/desk are dominated by category 3
(sphere/box/primitive operands), which R2 cannot touch and which has no
proposed remedy in this plan.

**Decision: proceed to Phase R2, skip R1.** R1 is closed as not-needed (no
category-2 population found). R2's push-down is the plan's only remaining
lever, and drums is exactly the scene it was designed for — half its
residual population converts to world-space quadrics on the first pass,
directly displacing the current single largest named hot cluster in the
whole Plan 5-8 sequence.

### Phase R2 — Attempted, disabled pending further debugging (2026-07-04)

Implemented in `BakedSceneBuilder.cpp`: `pushDownStepsIntoProgram` walks an
already-baked nested `CsgProgram` and, for every operand that is a quadric,
plane, or (recursively) an all-bakeable `NestedCsg`, re-bakes it from its
*raw* geometry with `childSteps ++ parentSteps` (childSteps first — verified
against the existing, working precedent in `bakeSimpleBody`'s
geometry-steps-then-body-steps fold, and against `Matrix4x4d`'s row-vector/
`v' = v*M` convention, which makes T_combined = T_child * T_parent, i.e.
encounter order). AABB bounds are moved into the parent's space in the same
pass via `AxisAlignedBox::fromTransformedCorners` on the parent's own
`objectToLocal` matrix (this part had no bugs). Wired into `bakeCsgOperand`
behind `kEnableNestedTransformPushdown`, gated on
`nestedProgramFullyBakeable`.

R0's own eligibility check had a bug, fixed before R2 landed: it accepted
any `Direct*`-kind operand as bakeable, but `Direct*` also covers
untransformed Sphere/Box/Blob (no coefficient congruence exists for those).
Fixed to check `quadricGeometry != nullptr || isInfinitePlane` directly;
drums's numbers were unaffected by the correction (all its Direct-kind
nested children happen to be quadric/plane already).

**Bug 1 (found, fixed): step accumulation across nesting levels.** drums.inc
has real 3-level chains (`Tensioner` → `Tensioner1` → `Disk_X`, each level
its own `TransformedNestedCsg`). Each level's push-down runs as a *separate*
call, bottom-up. The first implementation re-derived `combinedSteps` from
`operand.operand->getSteps()` (the leaf's own original, unchanging steps)
every time, so a second (outer) push-down pass silently discarded whatever
an earlier (inner) pass had already folded in — the leaf ended up baked
with only the outermost layer's steps, missing every intermediate layer.
Fixed by adding `CsgOperandRecord::pushdownAccumulatedSteps`, extended
(never re-derived from scratch) on each subsequent pass. Verified via a
hand-built 3-level test scene (`union{Tensioner1 rotate<> translate<>}`
pattern): rendered standalone, this now matches the pre-push-down reference
byte-for-byte (AE=0).

**Bug 2 (found, not yet fixed): touching/overlapping instances of the same
shared declared CSG.** The same 3-level test scene with *two* mirrored
instances present together (`union{Tensioner1 translate<A>} union{Tensioner1
rotate<> translate<B>}`, touching at a shared seam — exactly drums.inc's
`Tensioner` pattern) shows a small but real defect: thin dark crack lines at
the seam where the two instances meet, AE=524/120000 in the isolated test.
Each instance compiles to its own independent baked copy (confirmed:
`compileConstructiveSolidGeometry` never caches by geometry pointer, so
there is no shared-state corruption between the two `Tensioner1` copies),
so this isn't cross-contamination - it is the exact "candidate order/tie-
breaking at coincident surfaces" risk the plan's own Risks section named in
advance. Likely fixable but not attempted further this session.

**Net result: still far from acceptable.** After fixing Bug 1, a full-suite
run still shows large, structural (not cosmetic) damage - drums AE=680674/
1024000 (66% of pixels), plus **new** large regressions on chess (AE=8447,
up from 1), snail (AE=512532, up from 3), and tomb (AE=402581, up from
126406) that were *smaller* before the Bug 1 fix. This is not the shrinking
trend a converging fix should show; it means at least one more real,
undiscovered defect exists beyond Bugs 1 and 2, most likely in a CSG
population this session's hand-built test scenes didn't exercise (drums has
`composite`-nested CSGs, `difference` operators, and `Blob`/`HeightField`
operands that no isolated test above covered).

**Decision: R2 disabled** (`kEnableNestedTransformPushdown = false`),
verified gate green byte-identical to the Plan 8 Phase 1 exit state with it
off. The code stays in the tree, dead behind the flag, so a future session
does not have to re-derive the step-order proof, the AABB fix, or Bug 1's
diagnosis - only Bug 2 and whatever produces the still-growing full-suite
damage. This is exactly the kind of result Plan 9/10's "declare honestly,
do not grind on sunk cost" framing was written for: two real, non-obvious
bugs found and one fixed in a single session is a legitimate amount of
progress, but shipping R2 live would be a correctness regression, not a
performance win, and is refused on those grounds.

**Recommendation:** treat R2 as a distinct future sub-task with its own
budget, not a same-session continuation. Before resuming, add hand-built
test coverage for `difference`, `composite`-embedded CSGs, and blob/height-
field operands specifically (the isolated tests here only covered
`union`/`intersection` of quadric/plane), since the post-Bug-1 regression
pattern (three scenes getting *worse*) points at a population category
none of this session's tests touched.

### Refocused acceptance criteria

- Phase R0 table published for the six named scenes (hard requirement even
  if the kill criterion fires — the categorization is this plan's minimum
  deliverable).
- If R1/R2 proceed: drums quadric-test count through
  `sharesRaySpace=false` paths measurably reduced (gprof call counts), and
  drums wall-clock reduced accordingly; a FLOP-side change of this size not
  moving wall-clock would falsify the diagnosis and must be recorded as
  such.
- The original fusion criteria ("`traceOperandAllCrossings` deleted",
  "≥50% self-time in ≤10 functions") are **retired**, not inherited.

## Risks and Dead-End Reminders

- **Byte-exactness under reordering.** The recorded dead ends are explicit:
  append-only queues broke byte-exact; the Plan 3 clone-reduction emitter
  broke `takeoff.tga` via first-hit ordering. Fuse control flow, keep
  candidate order; when a kernel wants a better order, that is a Plan 9+
  experiment with its own gate, never a silent part of fusion.
- **Icache blowup.** 15+ fused kernels × aggressive inlining can exceed the
  icache the fusion was meant to relieve. Watch the panel (not just drums)
  at every phase; if a phase wins drums but loses the panel, split or
  de-inline that kernel.
- **Compiler-dependent wins.** Function-pointer vs switch dispatch and
  inline decisions must be measured on the Release toolchain used for the
  official numbers, not assumed.
- The mirror-corner bypass (`skyvase`) and nested-composite virtual dispatch
  (`pacman` eyes) are historic regression sites; both have golden coverage,
  but check them explicitly in the phase notes.
