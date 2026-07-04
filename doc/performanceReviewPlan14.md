# Performance Review Plan 14: Candidate-Traffic Reduction — Indirect Heap and Hot/Cold Data Split

## Position in the Plan Sequence

Third executable plan of the Plan 11 cycle; runs after Plan 13 because the
scan removal changes which traffic dominates (Plan 13 Phase 3's re-profile
is this plan's entry evidence). Targets the memory traffic that Plan 11
measured wrapped around every surviving intersection test.

## Evidence Being Attacked (from Plan 11 §2, drums)

- 24.7 M `PriorityQueue<IntersectionCandidate>::siftUp` calls, each moving
  **168-byte** elements (`Intersection` 56 B + `IntersectionAttributes`
  ~112 B, of which 64 B is `detailOwners[8]`, nearly always almost empty).
  Every `offer` copies the full candidate in, every extraction copies it
  out, every sift swaps pairs of them.
- `IntersectionCandidate` is also copied wholesale in the merge machinery
  (`mergeByMembership`, 5.4 M calls) and the first-hit paths.
- The Morgan-cycle functions pass through `CsgOperandRecord`s whose layout
  interleaves hot dispatch fields (kind, bounds, geometry pointer) with
  cold ones (two `Matrix4x4d` = 256 B, baked copies, debug fields), so the
  per-operand scan that survives Plan 13 still drags cold cache lines.

## Architectural Constraints

- All changes live in `render/bakedScene`, `environment/geometry/element`
  (the queue/candidate plumbing — ray *state*, not geometry math), and
  `base` container code. No transform knowledge moves anywhere.
- **Bit-identical extraction order is the design requirement, proven, not
  assumed.** The tie-break dead end ([[performance_plan9_staging_closed]])
  was about *changing* consumption order; this plan only changes element
  *representation* while keeping the heap algorithm, comparator inputs,
  and insertion sequence identical.

## Phase 0 — Candidate Anatomy and Copy Census (evidence, no code)

Instrument (temporarily) or reason statically over: candidates offered per
ray, average heap depth per sift, and which `IntersectionAttributes`
fields are populated at offer time vs. only at extraction winner time.
Key question for Phase 2: does anything read a *loser* candidate's
attributes after extraction? (Expected: no — losers are discarded.)

## Phase 1 — Indirect Heap (heap of indices over a stable arena)

1. Give the per-worker queue pool a parallel *arena*: a flat array of
   `IntersectionCandidate` (grown geometrically, but **entries never
   relocated during a drain** — grow by chunk-chaining or reserve-by-
   census, not by realloc-and-move; the Plan 5 relocation trap applies).
2. `offer` appends the candidate to the arena and pushes its index; the
   heap becomes `PriorityQueue<int>` (or a thin wrapper) whose `lessThan`
   compares `arena[a].t` vs `arena[b].t` — the *same doubles* in the same
   order as today. Sift swaps move 4-byte indices instead of 168-byte
   records.
3. Extraction resolves the index to the arena entry.

Byte-exactness argument: the heap algorithm (`siftUp`/`siftDown` code) is
unchanged; comparator operands are the identical `t` values; insertion
sequence is identical; therefore the extraction sequence — including
equal-`t` tie resolution, which depends only on algorithm + insertion
order — is identical. What changes is solely the width of the swapped
element. Prove it before shipping: a debug mode that runs both heap
representations side by side on the same offers and asserts identical pop
sequences across a full drums + chess render.

Keep the Plan 5-style kill-switch during the plan.

## Phase 2 — Slim the Candidate (only if Phase 0 says losers are write-only)

Split `IntersectionAttributes.detailOwners[8]` (64 B) out of the queued
record: candidates carry a small fixed header (t, point, hitGeometry,
material, flags, and a one-or-two-owner inline fast path — Phase 0's
census decides the inline count); the rare deep detail-owner stacks
(nested CSG shading unwind) spill to a per-worker side buffer referenced
by index. If the census shows the spill path would be hot, stop — do not
trade a copy for a pointer chase on the common path.

## Phase 3 — `CsgOperandRecord` Hot/Cold Split

Reorder/split the operand record so the fields the trace loop reads for
*every* operand (kind, `bounded`/`cullSafe`, `bakedBounds`, geometry
pointer, material pointer) sit in one compact array indexed alongside the
cold record (matrices, baked value copies, build bookkeeping). The Morgan
loop and Plan 13's structure then scan hot lines only. Pure layout change,
no numeric effect; gate must stay byte-identical.

## Phase 4 — Close

- Full gate, `renderParallel.sh` determinism (the arena is per-worker
  state — audit it the way [[parallel_raytracer_plan]] audited per-task
  TextureUtils), valgrind on chess.
- drums/iortest/panel/renderAll timing table vs Plan 13 exit; re-profile
  drums; publish.

## Acceptance Criteria

- Side-by-side heap-equivalence assertion passes on drums + chess full
  renders before the kill-switch is removed.
- Gate byte-identical throughout (zero re-baselines).
- Measurable drums improvement attributable to sift/copy cost (gprof
  `siftUp` self-time and candidate-copy share both down), or the phase
  reverted with the numbers recorded.

## Known Dead Ends (do not repeat)

- Changing heap consumption *order* or batching drains
  ([[performance_plan9_staging_closed]] — the one real batch site carries
  a tie-break risk; this plan must not recreate it).
- Append-only scratch queues ([[csgoperand_clone_perf_fix]]).
- Relocating arena entries mid-drain (same bug class as the Plan 5
  `ArrayList` copy-assign trap).

## Results (2026-07-04) — Researched, not implemented (by decision)

Phase 0 was executed in full (research + one temporary instrumentation
pass, reverted before close); Phases 1-3 were **not implemented**, each
for a distinct, evidence-based reason recorded below. No source file is
different from this plan's starting state - `git diff --stat` is empty
except for this doc.

### Phase 0 findings

- `IntersectionCandidate` = `Intersection` (56 B: `double t` + two
  `Vector3Dd`) + `IntersectionAttributes`. Measured `IntersectionAttributes`
  layout on this ABI (g++, x86-64, no packing pragmas): `hitGeometry`(8) +
  `hitBody`(8) + `detailOwners[8]`(64) + `detailOwnerCount`(padded to 8) +
  `material`(8) + `objectTexture`(8) + `objectColor`(8) + two bools
  (padded to 8) = **120 B**, so `IntersectionCandidate` is **176 B**, not
  the plan's estimated 168 B (close, not exact - immaterial to the
  conclusions below).
- Comparator (`IntersectionCandidate.h:49-55`,
  `PriorityQueue<IntersectionCandidate>::lessThan` specialization) is
  confirmed to compare **only** `t` - no secondary tie-break field exists
  anywhere in the codebase. This confirms Phase 1's byte-exactness
  argument (same comparator + same insertion order -> same extraction
  order regardless of element width) is sound *in principle*.
- `java::PriorityQueue<T>` (`base/src/main/java/util/PriorityQueue.txx`)
  is a classic 1-indexed flat-array binary heap; every `siftUp`/`siftDown`
  swap does a full by-value `T` copy (`const T tempEntry = data[index];`),
  exactly the traffic the plan targets. `PriorityQueuePool<T>` is
  confirmed per-task/per-worker state already (one pool per `RenderEngine`
  / per parallel task), so `-parallel` safety for an arena would follow
  the same existing lifetime discipline - not a new problem to solve.
- Only 2 read sites for `getDetailOwnerAt`/`getDetailOwnerCount` outside
  `IntersectionAttributes` itself, both in `RenderEngine.cpp` (511/514) on
  `localIntersection` **after** it has already been chosen as the winning
  hit (`out = depthQueue->peek()` / the winning candidate from the trace
  loop) - confirming Phase 0's expectation that losers' attributes are
  never read post-extraction.
- **New finding, not anticipated by the plan text**: `detailOwners` is
  NOT write-once-then-read-or-discard. `BakedCsgTrace::annotateDirectCandidates`
  (`BakedCsgTrace.cpp:529-550`) iterates `*depthQueue` directly
  (`for (IntersectionCandidate &candidate : *depthQueue)`) and calls
  `pushDetailOwner` on candidates **while they are still sitting in the
  queue**, before any winner/loser decision has been made. Any inline+spill
  redesign for Phase 2 therefore needs the spill arena reachable from
  every live in-queue candidate, not just the final winner - a materially
  larger scope than "spill at construction, read at extraction".
- Instrumented (temporarily, reverted before close) `pushDetailOwner`/
  `prependDetailOwner` with an atomic max-count tracker and ran the full
  `renderAll.sh` golden suite (108 sampled scene/pass instances). Real
  distribution of `detailOwnerCount` reached: `0:29 1:28 2:11 3:18 4:16
  5:2 6:2 7:2` (max observed: 7; hard cap is 8, never hit exactly). This
  rules out the naive "just shrink `MAX_DETAIL_OWNERS` from 8 to N<5" fix:
  6 of 108 samples (~6%, not "hot" by the plan's own Phase 2 criterion,
  but real) legitimately need 5-7 slots, and silently truncating them
  would change shading output - **not** byte-identical.

### Why Phases 1-3 were not implemented

- **Phase 1 (indirect heap of indices)**: `grep -rn "PriorityQueue<IntersectionCandidate>"`
  across `src/` returns **137 matches**. Converting the queue's element
  type from `IntersectionCandidate` to an arena index touches ~20 function
  signatures carrying `PriorityQueue<IntersectionCandidate> *depthQueue`,
  ~18-20 direct `offer`/`peek`/`poll` call sites, ~10 `borrowQueue`/
  `returnQueue` lifecycle sites, the comparator specialization, the
  `PriorityQueuePool`/`RayWithSegments` declarations, and at least one
  explicit template instantiation - essentially every function in
  `BakedTrace.{h,cpp}` and `BakedCsgTrace.{h,cpp}` that touches a depth
  queue. Given Plan 13's much smaller pointer-field change cost multiple
  hours of investigation (and turned out to include one multi-hour false
  alarm from a measurement-methodology artifact, see
  [[performance_plan13_phase1_methodology_lesson]]), the user and I agreed
  this refactor's risk-to-payoff ratio was poor enough to skip rather than
  attempt - discussed and confirmed with the user rather than assumed.
- **Phase 2 (slim the candidate)**: blocked on the same in-queue-mutation
  finding above. A correct implementation needs a per-ray spill arena
  reachable from every live candidate (not just winners), which means
  either storing an arena reference on every `IntersectionAttributes`
  (partially offsetting the size win) or threading an arena parameter
  through every `pushDetailOwner`/`prependDetailOwner` call site (~31
  call sites). Real measured depth data (above) also rules out the
  "just shrink the array" shortcut. Discussed and confirmed with the user:
  not attempted this session.
- **Phase 3 (`CsgOperandRecord` hot/cold split)**: not attempted (Phase 1/2
  were prerequisite context-setting per the plan's own phase ordering, and
  the session's remaining time went to confirming Phase 1/2's non-viability
  rather than starting a third large refactor). Flagged for whoever picks
  this up: `BakedSceneBuilder.cpp:1002-1030`'s fix-up pass sets
  `operand.geometry = &operand.bakedQuadricStorage` (a *self*-pointer into
  the same record), which a hot/cold split would need to re-target to
  `&cold[index].bakedQuadricStorage` - and would need the cold array's
  "never relocates after this point" guarantee (already relied on for the
  hot array, see the fix-up pass's own comment) extended to the new cold
  array as well.

### Acceptance Criteria status

Not applicable - nothing was implemented, so there is nothing to gate.
`git diff --stat` against this plan's starting commit shows only this
doc changed. No re-baselines, no gate risk, no code risk.

### Handoff

Plan 14 is CLOSED without code changes. The Phase 0 evidence above (struct
size, comparator behavior, detail-owner depth distribution, and the
in-queue-mutation finding) is the reusable asset for whoever revisits
candidate-traffic reduction later - it should save re-deriving the same
facts. drums/iortest's actual bottleneck (many small CSG program
dispatches, established across Plans 11-13) remains unaddressed by Plans
13 and 14 both; Plan 15 (composite transform coefficient collapse) and
Plan 16 (consolidation) are next in the sequence.
