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
