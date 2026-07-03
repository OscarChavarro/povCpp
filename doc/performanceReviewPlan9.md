# Performance Review Plan 9: Candidate Staging — Heap Traffic, Clone Residue, and Container Churn

## Position in the Plan Sequence

Fifth of six (Plans 5–10; sequence table in `doc/performanceReviewPlan5.md`).
Requires Plan 8: the fused kernels are the only consumers of the staging
machinery, so changing that machinery now touches a small, settled surface.
This plan is Plan 4 §6.4 and Strategy Implication #3, plus the clone residue
left after Plans 5–8.

## Motivation (from Plan 4, Conclusions §4 and Diagnostic Observation 3)

Queue overhead was 11.7% of total suite time: `PriorityQueuePool::push`
303.7 M / `pop` 295.3 M calls, `java::PriorityQueue::siftUp` 1.48%, plus
container churn tied to ray clones — `java::ArrayList::init` 260.9 M and
`ArrayList::ArrayList` 266.2 M calls track the clone count almost exactly
(each `RayWithSegments` clone builds ~200 bytes including two ArrayList
headers). Candidates were staged into scratch queues and re-offered into
parent queues, doubling heap traffic per hit relative to the baseline's
single-level Morgan local queue.

Plans 5–8 removed most clone *sources* and dispatch layers; this plan
attacks what staging remains:

1. heap insertion where order is not consumed incrementally,
2. double staging (scratch → parent re-offer),
3. the residual clones and their ArrayList construction cost.

All numbers above are pre-rebuild; **Phase 0 re-measures** and each phase
target is restated against the fresh profile before work starts.

## Architectural Constraints

- Work confined to `render/bakedScene` (kernels + scratch context) and
  `common/dataStructures` (only if a new container variant is added; the
  existing `PriorityQueuePool` template stays available for consumers that
  genuinely need incremental ordered consumption).
- Byte-exactness discipline changes in this plan: some phases *intend* to
  keep byte-identical output while changing containers (same comparison
  function, same tie-breaking); any phase that cannot guarantee identical
  tie-breaking must say so up front and gate against the golden protocol
  with explicit justification — silent acceptance of "close enough" is not
  allowed.

## Known Dead Ends (this plan's territory is mined — read first)

- **Append-only queues broke byte-exact** (CsgOperand clone perf fix cycle).
  Tie-breaking order of equal-`t` candidates is observable in output.
- **In-place scratch-queue drains were already implemented once** (same
  cycle, halving 168-byte candidate copies) — verify what survived the
  Plan 6 rebuild before re-doing it; the porting map from Plan 6 Phase 0
  says where it went.
- **Reusable local-ray scratch stacks** regressed wall-clock (~12.1 s era);
  **shared scratch ownership** likewise (~11.75 s). Pool lifetime changes
  need measurement, not intuition.
- Late shadow `minT`/`maxT` filtering after candidate generation: no win,
  slower shadow path.

## Phases

### Phase 0 — Fresh staging profile

gprof on drums + `iortest` + `piece2` (Group B representative) at the Plan 8
exit commit. Table: push/pop/siftUp/ArrayList counts and self-times, clone
count, and — new — a temporary instrumentation counter distinguishing
*ordered consumption* sites (pop consumed in `t` order incrementally) from
*batch consumption* sites (all candidates drained, then processed). The
counter classifies each kernel's staging use. Restate each later phase's
target numbers here.

### Phase 1 — Sort-on-demand vectors at batch-consumption sites

For kernels the Phase 0 classification marks batch-consuming: replace the
heap with a flat scratch vector + one `std::stable_sort` (stable = explicit
tie-breaking preservation; pair with the same comparator the heap used, and
verify the heap's effective tie order — a binary heap is *not* stable, so
"stable_sort with same comparator" can differ from heap pop order on ties.
If ties exist in practice (equal `t` from coincident surfaces — CSG scenes
have them by construction), matching heap order exactly may require sorting
by (t, insertion index) with the insertion index replicating heap tie
behavior — prototype on `drums` and `chess` first, gate byte-exact, and if
heap tie order proves unreproducible cheaply, keep the heap at those sites
and record it).

### Phase 2 — Eliminate double staging

Where a kernel stages into a scratch container only to re-offer everything
into the parent container, pass the parent container down (the fused kernels
of Plan 8 make ownership explicit, so this is now a mechanical change where
it was fragile before). Sites where scratch exists for rollback semantics
(candidate discarded if containment fails) keep their scratch but drain
in place.

### Phase 3 — Residual clone cost

For whatever `LocalIntersectionClone` population remains (residual
transformed operands from Plan 6 statistics): the clone's cost is dominated
by ArrayList header construction. Options in preference order: (a) a
pre-sized per-task clone slot in the scratch context reused across operand
visits (reset, not reconstruct — but see the dead-end list: measure against
the recorded scratch-stack regression), (b) a lighter local-ray struct for
kernel-internal use that carries only origin/direction/aggregates without
segment ArrayLists, if no callee needs segments (audit first). Gate each
option separately.

### Phase 4 — Container hygiene

`ArrayList` growth policies at hot sites (pre-size scratch vectors to the
per-scene high-water mark recorded by the pool — the pool already grows to
it; make the initial size a builder statistic instead of a runtime ramp),
`dispose`/destructor churn (23.1 M + 46.2 M calls pre-rebuild). Only sites
the Phase 0 profile still shows; this phase is deliberately last because
Plans 5–8 likely shrank it.

### Phase 5 — Closing measurement

drums (3 runs), panel, gprof, `-parallel` determinism check.

## Measurement Gate

Every phase:

```bash
./scripts/clean.sh
./scripts/compile.sh
./scripts/renderAll.sh
./scripts/testAgainstGoldenImages.sh
```

Byte-identical vs the Plan 8 exit state is the default requirement; any
phase that changes candidate ordering semantics must instead invoke the
golden evaluation protocol from Plan 5 with explicit before/after diffs and
a written justification, and is rejected if any scene shows structured
(yellow/red heat-map) change. Plus drums timing per phase; panel + gprof at
Phases 1, 3, 5.

## Acceptance Criteria

- push/pop volume on drums reduced by the batch-consumption share measured
  in Phase 0 (record actual %).
- Clone count on drums at or near the irreducible residual-transformed
  operand count; ArrayList init/ctor counts no longer track clones 1:1.
- drums wall-clock: strictly no regression; expected single-digit-% gain
  (queue layer was 11.7% suite-wide pre-rebuild; drums-specific share to be
  restated in Phase 0).
- Panel: no scene regresses > 2%.

## Risks

- This is the plan with the highest historical failure density (see the
  dead-end list). The defense is Phase 0's consumption classification: only
  touch sites *proven* batch-consuming, and gate every phase separately.
- Tie-breaking: treat any `AE≠0` on a previously byte-exact scene as an
  ordering bug, not as noise, even when the image "looks identical".
