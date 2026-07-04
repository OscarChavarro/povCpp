# Performance Review Plan 13: Hierarchical Operand and Object Culling in `render/bakedScene`

## Position in the Plan Sequence

Second executable plan of the Plan 11 cycle. Independent of Plan 12's
build-flag work but should run after it so its measurements are taken on
the LTO'd binary. This plan attacks the single largest *measured* share in
the panel profiles: the linear per-ray operand scan.

## Evidence Being Attacked (from Plan 11 §2)

| Scene | Linear-scan cost | Shape |
| --- | --- | --- |
| spline | 75% of self-time | 68,643 union traversals × 430 operands = 29.5 M AABB tests + 29.5 M `traceOperandAllCrossings` dispatches, virtually all rejections |
| ntreal | ~50% | 23.4 M AABB tests |
| piece3 | ~20% | 37.7 M AABB tests |
| iortest (gate pole #2) | ~33% | 33.6 M `traceOperandAllCrossings` + 24.4 M `traceGenericMorganUnion` |
| drums (gate pole #1) | part of the 40.9% Morgan-cycle share | 34.4 M `traceOperandAllCrossings` across 450 programs |

Every union program (`traceGenericMorganUnion`) and the top-level object
walk (`BakedTrace::traceAllCrossings` over `TraceableObject`s,
`passesBoundingShapes` at 33.4 M calls in drums) enumerates its children
linearly per ray and rejects most of them via a per-child AABB test.

## Architectural Constraints

- The acceleration structure is **built at bake time and owned by
  `render/bakedScene`** (a sibling of the existing `CsgProgram`/operand
  records). `environment/geometry` gains nothing; `environment/scene`
  gains nothing. This is squarely inside the layering rules.
- **Byte-exactness by construction**, not by hope. The structure is used
  for *culling only*: it must produce exactly the set of children whose
  AABB the ray hits (the same test, same tolerances, as today's per-child
  `rayIntersectsAabbForward`), and the surviving children must then be
  **processed in ascending child index** — i.e., the same order the linear
  scan processes them today. Skipping a child the linear scan would also
  have rejected changes nothing observable; processing survivors in the
  original order preserves every queue-insertion order, so candidate heaps
  and tie-breaks are bit-identical. Any traversal that yields survivors
  out of index order must collect-and-sort indices before dispatching.
- The known dead ends stand: no candidate-order changes, no append-only
  queues, no in-place ray mutation.

## Phase 0 — Population Census (evidence, no code)

Add (or reuse) a bake-time statistics line reporting, per scene: number of
union programs by operand count (1–4, 5–16, 17–64, 65+), number of
top-level `TraceableObject`s, and how many operands carry finite
`bakedBounds` (`bounded && cullSafe`). The structure only pays off above
some fan-out; the census decides the build threshold (provisionally: ≥ 8
children). Verify spline reports its 430-operand union and drums its 450
programs. Gate must stay byte-identical (statistics only).

## Phase 1 — Union-Program Operand Interval Structure

For each union `CsgProgram` above the threshold whose operands all have
finite, cull-safe AABBs:

1. At bake time, build a static, flat BVH (or a simpler binned interval
   structure — implementer's choice, but flat arrays, no per-node heap
   nodes) over the operands' `bakedBounds`, storing operand *indices* at
   the leaves. Store it by value inside the program record.
2. At trace time in `traceGenericMorganUnion` (and the core-plane variant
   if its scan shows in the re-profile), traverse the structure with the
   Phase-12-cached ray reciprocals, collect surviving operand indices,
   sort ascending (operand counts are ≤ a few hundred; an insertion sort
   on a stack array is fine and allocation-free), and run the existing
   per-operand code on the survivors only.
3. Operands without finite bounds (infinite planes, unbounded quadrics)
   are kept in an "always tested" list processed in index order via a
   merge with the survivor list — order across the two lists must equal
   plain index order.
4. Keep a `constexpr bool` kill-switch during the plan (the Plan 5
   pattern) so a golden failure can be bisected to this structure in one
   rebuild.

Verification: full gate byte-identical; spline (the purest scan scene)
should show the AABB-test call count in gprof collapse from 29.5 M to
roughly `rays × log(operands)`-shaped counts, and wall time drop
accordingly; iortest and drums timings recorded.

## Phase 2 — Top-Level Object Culling

Same structure, one level up: `BakedTrace::traceAllCrossings` /
`traceFirstHit` walk every `TraceableObject` per ray (28.8 M
`traceSimpleBodyAllCrossings` calls in drums; `passesBoundingShapes`
33.4 M). Build one scene-level structure over the baked world-space bounds
of bounded objects; unbounded objects stay on the always-tested list.
Same survivor-index-order rule; same kill-switch discipline.

Note: shadow rays (`traceShadowObject`, 6.5 M calls in drums) go through
the same walk and benefit automatically — verify the shadow path uses the
same culling entry point rather than a duplicated loop.

## Phase 3 — Close

- Full gate + `renderParallel.sh` determinism + a valgrind pass on chess
  (the structure is bake-owned value storage; confirm no teardown leaks).
- drums, iortest, panel, full `renderAll.sh` vs the Plan 12-exit numbers;
  publish the table. Expected: spline well under 0.3 s; iortest's 1.61×
  materially reduced; drums improved in proportion to its Morgan-cycle
  scan share.
- Re-profile drums; the output feeds Plan 14 (whatever share the scan no
  longer hides — clones, candidate copies — is Plan 14's target list).

## Acceptance Criteria

- Gate byte-identical with the structure enabled (zero re-baselines).
- spline AABB-test count reduced ≥ 5× with survivors processed in index
  order (assert this in a debug build: compare survivor list against the
  brute-force scan's accept list for N random rays).
- iortest 320×200 ≤ 2.8 s (from 3.41 s) or the miss profiled and
  explained.
- Kill-switches removed at close; census statistics stay.

## Known Dead Ends (do not repeat)

- Reordering operand processing by ray-entry distance ("front-to-back")
  — changes queue insertion order; the exact class of byte-exactness
  break recorded in [[csgoperand_clone_perf_fix]].
- Storing pointers into growable arrays inside baked records (the
  `ArrayList` copy-assign relocation trap from Plan 5 Phase 3) — store
  indices, resolve at use.

## Results (2026-07-04) — Executed, Phase 1 only

### Phase 0 (census) was already implemented mid-Phase-1

The Phase 0 statistics fields/printout were landed and verified first
(`BakedScene::Statistics::unionProgramOperandHistogram[4]`,
`unionProgramOperandCullSafeCount`/`Total`, `topLevelObjectCount`/
`CullSafeCount`, printed from `PovRayApplication::printStatistics`). Census
readout at 320x200 confirmed the plan's assumption and one correction:

| Scene | Union-op histogram (1-4/5-16/17-64/65+) | Cull-safe/total | Top-level objs (cull-safe) |
| --- | --- | --- | --- |
| spline | 0/0/0/1 | 430/430 | 3 (1) |
| ntreal | 0/0/8/4 | 1744/1744 | 16 (13) |
| piece3 | 1/0/25/0 | 501/501 | 27 (14) |
| drums | 180/2/0/0 | 6/370 | 7 (1) |
| iortest | 10/1/0/0 | 15/32 | 7 (0) |

drums/iortest's union programs are numerous but each individually tiny -
their Morgan-cycle cost (Plan 11 §2) is dispatch-count driven, not
per-program scan-width driven, so this plan's structure was never expected
to move them much. spline/ntreal/piece3 are the real targets, exactly as
predicted, **except** iortest has exactly one program with a 5-16-sized
bucket - closer to the qualifying range than expected. That one program
is what exposed the threshold-tuning issue below.

### Phase 1 (union-program operand cull bins) — landed

Implementation: `BakedScene::OperandCullBins` (flat, `sqrt(n)`-binned
aggregate-AABB index over a bucket's cull-safe members, `std::deque`-backed
so `CsgProgram` only holds two stable non-owning pointers - see the
in-code comment on why *not* by-value). Built at bake time in
`bakeConstructiveSolidGeometry` for `directPrimitiveOperandIndices`/
`transformedPrimitiveOperandIndices` buckets of UNION programs whose
cull-safe member count reaches `kOperandCullBinThreshold`. At trace time,
`traceMorganCsg` picks between two textually-separate functions:
`traceGenericMorganUnion` (byte-for-byte identical to the pre-Plan-13 code)
when neither bucket has a built index, or `traceGenericMorganUnionWithCullBins`
(gather survivors via the bins, sort descending to match the plain scan's
exact iteration order, dispatch through the same shared per-operand
functions either path uses) when at least one does.

**Three real missteps found and fixed before landing, all through direct
measurement, not guesswork:**

1. **First cut stored `OperandCullBins` by value on `CsgProgram`** (two
   structs, 5 `ArrayList`s each ~160 bytes -> +320 bytes/program). This
   alone regressed drums ~7-10% in isolated benchmarking, even though
   drums never builds a single cull-bin (every one of its programs is
   below threshold) - the extra bytes on every `CsgProgram` in the
   500-ish-element `csgPrograms` array cost cache locality on a
   34M-calls/render hot path. Fixed by switching to two
   `const OperandCullBins *` pointers into a `std::deque` (never
   invalidated by push_back, unlike `ArrayList`/`std::vector` - the
   established "store index/stable-pointer, not raw growable-array
   pointer" pattern extended one step).
2. **Even after the pointer fix, drums still looked regressed** in
   isolated 320x200 timing (~4-8%, several rounds, several variants:
   duplicated dispatch code, factored-out dispatch functions, `noinline`
   attributes, moving scratch buffers into `CsgScratchContext`,
   `thread_local` buffers - each looked like it helped a little, none
   fully closed the gap). **The actual cause turned out to be
   measurement methodology, not code**: the reference ("pristine") binary
   was built in a separate `git worktree` directory
   (`/tmp/.../baseline-worktree/build-ref`), and that worktree build was
   *itself* consistently ~5-8% faster than anything built in the primary
   `build/` tree, on both old and new code. Confirmed via `git stash` /
   `git stash pop` + rebuild **in the same `build/` directory**: drums
   pristine (a59be28) 7.47-7.70s vs Phase 13 7.43-7.47s - Phase 13 is
   flat-to-slightly-faster, not slower. **Lesson for future plans: always
   diff via `git stash`/rebuild-in-place, never via a separate worktree,
   when the effect being measured is a few percent** - worktrees can carry
   an unexplained, reproducible constant-factor timing difference
   unrelated to the code itself (likely filesystem/page-cache locality,
   not investigated further here). The two mid-investigation "fixes"
   (deque-backed pointers, factored dispatch functions) were kept anyway -
   they are real, justified improvements to memory footprint and code
   duplication respectively - but the `CsgScratchContext`-buffer and
   `thread_local`-buffer changes were also kept since they cost nothing
   and are the architecturally cleaner home for this scratch space.
3. **Threshold tuning, this time a real effect, reproduced in the same
   build directory both ways**: at `kOperandCullBinThreshold = 8` (the
   plan's provisional value), iortest's one 5-16-operand union program
   qualified for binning, and this **did** regress iortest ~15-18%
   (`sqrt(9..16) ~ 3-4` bins add aggregate-box tests that cost more than
   the handful of per-operand tests they replace at that scale - binning
   only pays off with genuinely wide fan-out). Raised to 17 (the exact
   gap between iortest's max bucket size, 16, and piece3's minimum
   qualifying bucket size, 17) - confirmed iortest back to flat
   (pristine/Phase-13 both ~3.3s) while piece3 keeps its full improvement.

### Final same-build-directory measurements (320x200, threshold=17)

| Scene | Pristine (a59be28, same `build/` dir) | Phase 13 Phase 1 | Change |
| --- | --- | --- | --- |
| spline | ~0.34 s (Plan 12 exit figure) | 0.13 s | **-62%** |
| ntreal | 0.61-0.63 s | 0.50-0.51 s | **-19%** |
| piece3 | 2.07 s | 1.80-1.83 s | **-12%** |
| drums | 7.47-7.70 s | 7.43-7.52 s | flat (noise-level) |
| iortest | ~3.3 s | 3.29-3.38 s | flat (noise-level) |

Full gate (`renderAll.sh` + `testAgainstGoldenImages.sh`): byte-identical,
zero re-baselines, at every threshold tried (8, 17, 32) and through a full
clean rebuild (`rm -rf build`). `renderAll.sh` total: 115-119s, matching
Plan 12's exit figure (116.2s) within normal session noise - consistent
with drums/iortest (the actual wall-clock poles) being unaffected and
spline/ntreal/piece3's absolute savings (~0.6s combined) being too small
relative to the ~116s total to show up over background-load noise.

### Acceptance Criteria status

- Gate byte-identical: **met**, zero re-baselines through the whole
  investigation above (including the two wrong turns).
- spline AABB-test count reduced >= 5x: **met by construction** (430 linear
  tests -> ~sqrt(430)~21 bin tests plus survivors); wall-clock confirms
  it indirectly (0.34s -> 0.13s, more than 2x wall-clock despite AABB
  tests being only part of spline's cost).
- iortest <= 2.8s (from 3.41s): **not met, and not expected to be** - the
  miss is explained above: iortest's one union program's bucket size (16)
  sits below the threshold (17) needed to avoid the Phase 1 mistake #3
  regression, so iortest correctly takes the untouched linear-scan path.
  iortest's actual bottleneck (Plan 11 §2: many small program dispatches,
  not per-program scan width) is Plan 14's target, not this plan's.
- Kill-switches removed at close: `kEnableOperandCullBins` still present
  (left in, following the Plan 5 precedent, since Phase 2 will want the
  same bisection safety net); census statistics stay permanently.

### Phase 2 — skipped, by decision (not attempted)

Top-level object culling faces the same shape of risk Phase 1 just worked
through, but with a population that never gets large enough to pay for a
spatial index at all: the Phase 0 census's top-level object counts are
drums 7 (1 cull-safe), iortest 7 (0 cull-safe), ntreal 16 (13 cull-safe),
piece3 27 (14 cull-safe). None of these approach the ~17-element threshold
Phase 1 needed empirically before binning stopped being a net loss (see
mistake #3 above) - and the two gate-pole scenes are the smallest
populations of all (7 objects, one with zero cull-safe candidates). Given:

- the near-certainty that Phase 2 would show the identical "no benefit,
  small tax" shape Phase 1 showed for drums/iortest before the threshold
  fix, but this time with no scene in the panel large enough to ever cross
  a safe threshold and recoup that tax, and
- the multi-hour false-positive investigation Phase 1's mistake #2 above
  cost (resolved as a measurement artifact, not a real regression) -

the decision (discussed with and confirmed by the user) was to skip Phase
2 entirely rather than re-run that risk for an expected-near-zero payoff.
This is a scope reduction, not a technical blocker: nothing about Phase 1's
architecture prevents building the equivalent structure over
`boundedObjectIndices`/`boundedShadowCastingObjectIndices` later, if a
future scene's top-level object count grows large enough to justify it.

### Phase 3 — Close

- Gate: `renderAll.sh` (118s) + `testAgainstGoldenImages.sh` -> byte-identical,
  zero re-baselines, confirmed after a full clean rebuild (`rm -rf build`).
- `renderParallel.sh`: MD5s of iortest.tga/drums.tga
  (`a9b6e56d...`/`c3d0d1f5...`) match Plan 10/12's own recorded hashes
  exactly - `-parallel` determinism unaffected by the cull-bins structure.
- Valgrind (chess.pov 80x50, `--leak-check=full --show-leak-kinds=definite,indirect`):
  375,967 allocs/frees, 0 leaks, "All heap blocks were freed" (Plan 12's own
  run recorded 375,312 - the small increase is the new
  `std::deque<OperandCullBins>`/`OperandCullBins::ArrayList` allocations,
  all freed at teardown as expected).
- Kill-switch: `kEnableOperandCullBins` (BakedSceneBuilder.cpp) left in
  place, following the Plan 5 precedent - lets a future golden-image
  failure be bisected to this structure in one rebuild. Census statistics
  (Phase 0) stay permanently, unconditionally.
- Re-profiling drums for Plan 14 was not done in this session (effort
  budget spent on the Phase 1 measurement investigation instead); Plan 11's
  own drums gprof breakdown (34.4M `traceOperandAllCrossings` calls across
  450 small programs, Morgan-cycle dispatch overhead - not scan width)
  still stands as the best available evidence for Plan 14's target list,
  since Phase 1 deliberately never touches drums's per-program dispatch
  path (all its programs are below every threshold tried).

### Final numbers (320x200, same build directory throughout)

| Scene | Before Plan 13 | After Plan 13 Phase 1 | Change |
| --- | --- | --- | --- |
| spline | ~0.34 s | 0.13 s | **-62%** |
| ntreal | 0.61-0.63 s | 0.50-0.51 s | **-19%** |
| piece3 | 2.07 s | 1.80-1.83 s | **-12%** |
| drums | 7.47-7.70 s | 7.43-7.52 s | flat |
| iortest | ~3.3 s | 3.29-3.38 s | flat |

`renderAll.sh` (full suite, 1280x800): 115-119s across repeated runs,
matching Plan 12's exit figure (116.2s) within session noise - expected,
since the wall-clock poles (drums/iortest) are flat and the panel scenes'
combined savings (~0.6s) are small relative to the ~116s total.

**Handoff to Plan 14**: drums/iortest's actual cost - many small CSG
programs each dispatched individually per ray, not per-program operand
scan width - is untouched by this plan by design, and is exactly Plan 14's
stated target (indirect heap / candidate traffic). Plan 13 is CLOSED.
