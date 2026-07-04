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
