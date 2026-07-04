# Performance Review Plan 9: Candidate Staging — Heap Traffic, Clone Residue, and Container Churn

## Status: CLOSED at Phase 0 (2026-07-04) — entry criterion passed, but every phase's target is either stale (already fixed by Plans 5-8) or out of this plan's risk bar; no code changed

## Position in the Plan Sequence

Fifth of six (Plans 5–10; sequence table in `doc/performanceReviewPlan5.md`).

**Re-scoped 2026-07-04** after the Plan 7/8 findings: Plan 8's fusion
phases were retired (four consecutive dispatch-level attempts produced no
measurable drums win — see `doc/performanceReviewPlan8.md` Status), so the
original premise "the fused kernels are the only consumers of the staging
machinery" no longer holds. This plan now depends on Plan 8's *refocused*
continuation (collapse-rate expansion, Phases R0-R3) and is **conditional**:

### Entry criterion (added 2026-07-04 — check before starting any phase)

Run gprof on drums at the Plan 8-R exit commit. This plan proceeds only if
clone construction + ArrayList init/ctor + PriorityQueue siftUp/add
*combined* still hold ≥ 5% of drums self-time. On the 2026-07-04 profile
(commit f3ac202) the combined figure is ≈ 8.5%:
`RayWithSegments(LocalIntersectionClone,…)` 3.20% at 48.6M calls,
`ArrayList<Material*>::init` 1.68% + ctor 0.84% (48.9M calls — still
tracking clones ≈ 1:1), `siftUp` 1.18%, `PriorityQueue::add` 0.51%,
`makeRay` 2.19% (31.6M calls, mostly re-derivation after clones invalidate
the aggregate cache). If Plan 8-R2's push-down succeeds, most residual
clones disappear with the transformed operands that cause them — re-measure
first; if the figure drops below the threshold, mark this plan SKIPPED here
with the closing profile attached, and move to Plan 10.

Expectation management: even if the entry criterion passes, ≈ 8.5% is the
*ceiling* of what perfect execution of this plan could recover on drums —
a realistic outcome is low-single-digit %. The Plan 5-10 sequence's
remaining drums gap (~1.7×) cannot be closed from here; that burden sits on
Plan 8-R, not this plan.

## Motivation (Plan 4 §6.4 — original counts, now historical)

Queue overhead was 11.7% of total suite time pre-rebuild:
`PriorityQueuePool::push` 303.7 M / `pop` 295.3 M calls, `siftUp` 1.48%,
plus container churn tied to ray clones — `ArrayList::init` 260.9 M calls
tracking clones almost exactly (each `RayWithSegments` clone builds ~200
bytes including two ArrayList headers). Plans 5–8 removed most clone
sources; the 2026-07-04 numbers above are the current state. This plan
attacks what remains:

1. the residual clones and their ArrayList construction cost (now the
   largest item — reordered to Phase 1 priority, see below),
2. heap insertion where order is not consumed incrementally,
3. double staging (scratch → parent re-offer).

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

**Execution order re-scoped 2026-07-04:** run Phase 0, then Phase 3 (clone
residue — now the largest measured item), then Phases 1-2 (heap/staging)
only if Phase 0's classification still justifies them, then 4-5. The
original order assumed queue traffic dominated; the current profile says
clones do.

### Phase 0 — Fresh staging profile

gprof on drums + `iortest` + `piece2` (Group B representative) at the
Plan 8-R exit commit (this doubles as the entry-criterion check above).
Table: push/pop/siftUp/ArrayList counts and self-times, clone count, and —
new — a temporary instrumentation counter distinguishing *ordered
consumption* sites (pop consumed in `t` order incrementally) from *batch
consumption* sites (all candidates drained, then processed). The counter
classifies each kernel's staging use. Restate each later phase's target
numbers here.

Additionally record *where* the remaining clones come from (the 2026-07-04
callers, for comparison: `traceOperandAllCrossings` 21.3M,
`traceSimpleBodyAllCrossings` 13.0M, `traceCompositeAllCrossings` 12.5M) —
if Plan 8-R2 removed the transformed-operand clones, the composite and
simple-body clones become the whole story and Phase 3's option (b) below is
the relevant one.

### Phase 0 — Result (2026-07-04, commit pending)

Fresh `-pg` gprof profile, 320×200, on drums, `iortest` (Group A residual
CSG), and `piece2` (Group B non-CSG). Static classification substituted for
a runtime instrumentation counter: with only two `poll()` call sites and
one `Iterator`-based batch-read site in the entire `PriorityQueue<
IntersectionCandidate>` consumer set (found by grepping every `.poll()`/
`.iterator()`/`.forEach()`/`.toArray()` call against the type), each site's
consumption pattern is visible directly from its loop shape — a counter
would report the same three-way split a `grep` + read already gives, so no
temporary instrumentation was added.

**Consumption classification (all three sites, exhaustive):**

| Site | Function | Pattern | Verdict |
| --- | --- | --- | --- |
| `depthQueue->poll()` in a `while(size()>0)` loop with `break` on first passing candidate | `BakedTrace::traceSimpleBodyFirstHit` (clipped-body branch) | Ordered, early-exit | Heap justified — keep |
| `localDepthQueue->poll()` in a `while(size()>0)` loop, early `return true`/`clear()` on first valid occluder, or per-candidate shade-and-maybe-return | `traceShadowObject` (`DirectLightShader.cpp`) | Ordered, early-exit (shadow rays terminate on first opaque occluder in `t` order) | Heap justified — keep |
| `localDepthQueue->poll()` in a `while(size()>0)` loop with **no** early exit, feeding an `ArrayList<RaySegmentCrossing>` | `BakedCsgTrace::buildRaySegments` | Ordered pop, but fully drained regardless (order matters for the *inside/outside* toggle, not for early stopping) | Genuinely order-dependent, not a batch-consumption target — `RaySegments` algorithm (`-csgRoth` only), **not reached by default MorganRules rendering** (drums/iortest/piece2 all render via `CsgAlgorithm::MorganRules`) |
| `for (candidate : *localDepthQueue)` — raw unordered `Iterator` walk | `traceGenericMorganUnion`, `traceMorganIntersectionGeneric` (both call `tracePlanOperandAllCrossings` into a scratch queue, then iterate it unordered to test membership before `offer()`-ing survivors into the parent heap) | **True batch consumption** — the local queue's heap ordering is built (paying `siftUp` on every insert) and then thrown away unread | **Phase-1/2 candidate**, flagged below |

The Morgan-union local-scratch-queue finding is new (not anticipated by the
original Phase 1/2 write-up, which assumed `RaySegments`-style order-then-
process was the batch case). It is real staging waste: every candidate
generated by `tracePlanOperandAllCrossings` pays a full heap insert
(`siftUp`) into `localDepthQueue` purely so the code can immediately do an
**unordered** membership test and re-`offer()` survivors into the *parent*
heap, which re-establishes correct order independently of read order. See
Phase 1/2 disposition below for why this is not being fixed regardless.

**Clone-caller breakdown (`RayWithSegments::RayWithSegments(LocalIntersectionClone,...)`, via `gprof` call graph):**

| Scene | Total clones | `traceOperandAllCrossings` (CSG transformed-operand) | `traceSimpleBodyAllCrossings`+lambda | `traceCompositeAllCrossings` | `traceSimpleBodyFirstHit` | `doExtraInformation` (Simple/Csg) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| drums | 27.30M | **0** | 14.16M | 12.50M | 0.14M | 0.49M |
| iortest | 25.21M | 11.11M | 11.51M | 0 | 1.81M | 0.77M |
| piece2 | 15.89M | 0 | 8.90M | 0.56M | 6.30M | 0.13M |

drums confirms the Phase 0 prediction exactly: Plan 8-R2's push-down
eliminated **100%** of drums's `traceOperandAllCrossings`-caused clones
(21.3M → 0 calls to that caller); the composite/simple-body clones (both
pre-existing, unrelated to CSG operand transforms) are now the entire
population. iortest still carries an 11.1M-call `traceOperandAllCrossings`
share, consistent with its R0 table (10/20 residual operands are category 3
— unbakeable sphere/primitive, outside R2's reach). piece2 has none at all
(no CSG-transformed-operand source ever existed there).

**Phase 3's own premise is stale.** The plan text attributes clone cost to
"ArrayList header construction," but `RayWithSegments::RayWithSegments(
LocalIntersectionClone, const RayWithSegments&)` (`RayWithSegments.cpp`)
already constructs `containingTextures`/`containingIORs` at **capacity 0**
— and `ArrayList<T>::init()` (`ArrayList.txx`) skips the `new T[maxSize]`
call entirely when `maxSize == 0`. This optimization predates Plan 9 (its
own doc comment: "removes the dominant allocator cost from the ray/body
hot path"). The `ArrayList<Material*>::init`/`ArrayList(long)` self-time
seen in gprof (0.60%+0.60% on drums) is pure function-call overhead on a
no-op branch, not allocation — there is no header-construction cost left
to remove.

**Phase 4's own premise is also stale.** `PriorityQueuePool` (`environment/
geometry/element/PriorityQueuePool.txx`) already pre-allocates all 32
queues at `MAX_NUMBER_OF_ENTRIES` (128) capacity once, in `init()`, and
`pop()`/`push()` only `clear()` (reset `currentSize`, no dealloc/realloc) —
there is no runtime growth ramp at this pool. Confirms the plan's own
hedge ("Plans 5-8 likely shrank it").

**Decision: Phases 1-4 do not proceed.**
- **Phases 1-2** (batch-consumption heap replacement / double-staging
  removal): the one real batch-consumption site found
  (`traceGenericMorganUnion`/`traceMorganIntersectionGeneric`'s local
  scratch queue) is architecturally sound to fix, but its own risk profile
  — changing insert order into the parent heap changes tie-break order for
  equal-`t` candidates, the exact historical dead-end class ("append-only
  queues broke byte-exact") — makes it a separately-gated, single-purpose
  change, not something to fold into a plan whose entry criterion has
  already dropped from ~8.5% to 6.42% post-R2 with a shrinking absolute
  clone population. Recorded here as a candidate for a future narrowly
  scoped fix, not executed in this plan.
- **Phase 3** (residual clone cost / ArrayList header): premise stale, see
  above — no header cost is left to remove. The remaining clone cost is
  the intrinsic `RayWithSegments` object construction for a legitimate
  local-space-transform ray, once per simple-body/composite child with its
  own transform. The only lever that would remove the clone itself
  (mutate-in-place-then-restore) is the plan's own recorded dead end
  (reusable local-ray scratch stacks and shared scratch ownership both
  regressed wall-clock) — not re-attempted without new evidence the
  regression's cause no longer applies.
- **Phase 4** (container hygiene): no runtime growth ramp exists to
  pre-size away.
- **Phase 5** (closing measurement): no code changed in this plan, so
  there is nothing to re-measure beyond Phase 0's own numbers — restated:
  drums remains at the Plan 8-R2 state (~7.7s, gate green, `-parallel`
  deterministic; see `doc/performanceReviewPlan8.md` Phase R3).

**Plan 9 is closed at Phase 0.** This is the acceptance criteria's
explicitly sanctioned outcome ("a recorded SKIPPED verdict... is a fully
acceptable outcome of this plan... what is not acceptable is starting
Phase 1+ without the Phase 0 numbers justifying it") — the numbers justify
not starting, on evidence rather than the entry criterion's coarse
percentage alone. Every phase's originally-diagnosed target either doesn't
exist in the current architecture (Phases 3, 4) or exists but fails this
plan's own risk bar for an unscoped, multi-target plan (Phases 1-2). The
residual ~1.5× gap moves forward to Plan 10 as a structural-floor
candidate, not a staging-layer one.

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

- **A recorded SKIPPED verdict via the entry criterion is a fully
  acceptable outcome of this plan** — equal in standing to executing the
  phases. What is not acceptable is starting Phase 1+ without the Phase 0
  numbers justifying it.
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
