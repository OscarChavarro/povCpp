# Performance Review Plan 15: Coefficient Collapse Through Composite Transforms

## Position in the Plan Sequence

Fourth executable plan of the Plan 11 cycle, deliberately last of the
optimization plans: it is the only one with real numeric risk (it extends
Plan 5's replay machinery to a new owner class and therefore re-enters the
Golden-Image Evaluation Protocol world). Requires Plans 12–14 measured
first so its own effect is attributable.

## Evidence Being Attacked (from Plan 11 §2, drums)

- 14.9 M `traceCompositeAllCrossings` calls constructing **12.5 M**
  composite-space `LocalIntersectionClone` rays per render — about half of
  drums' 27.3 M total clone volume. `traceCompositeAllCrossingsInComposite
  Space` alone is 7.9% of drums self-time.
- 13.0 M of the remaining clones come from `traceSimpleBodyAllCrossings`
  on transformed bodies whose geometry kinds Plan 5 could not bake
  (spheres, boxes) or whose owners are composite children Plan 5 never
  reached.
- This is precisely Plan 10's residual suspect (d) — "structural clone
  volume independent of CSG population" — scoped down to its *bakeable
  subset*: composite children whose geometry is a quadric or plane with a
  recorded elementary-step list. The unbakeable remainder (spheres — see
  Plan 5 Phase 0: the baseline itself never coefficient-baked spheres —
  boxes, patches) stays declared structural floor.

## Architectural Constraints

- Identical to Plan 5's, verbatim: `environment/geometry` gains no
  transform knowledge, no mutable state, no baking hooks;
  `environment/scene` gains only passive parse-time data (one new
  `TransformStep` list per `Composite` level); all replay and all baked
  copies live in `render/bakedScene`.
- The Plan 5 numeric law stands: **replay elementary steps one at a time,
  in recorded order, with the exact baseline formulas** — never the
  composed matrix (recorded dead end).
- Every collapsed owner gets an explicit `bakedTransformFolded`-style flag
  set at bake time. Never infer "was baked" from pointer comparisons in
  `doExtraInformation` (twice-recorded dead end, Plan 5 Phases 3–4).

## Phase 0 — Composite Transform Semantics (evidence, no code)

Plan 5 Phase 1 deliberately did **not** record `TransformStep`s on
`Composite` (its ownership appendix: composites only ever apply
object-level transforms and were not a bake target). That decision is the
first thing to re-verify against today's code:

1. Trace how a `Composite`'s transform pair is built at parse time
   (`parseComposite` path) and how `traceCompositeAllCrossings` /
   `…InCompositeSpace` apply it per ray, including **nested** composites
   (the level-by-level model confirmed in Plan 5's ownership appendix and
   the [[nested_composite_virtual_dispatch_fix]] history — outer composite
   transforms must reach nested children).
2. Write down, in this document, the exact per-level replay order needed
   for a child quadric/plane: child geometry-layer steps, then child
   object-layer steps, then each enclosing composite's steps innermost-to-
   outermost — *derived from the matrix code, not assumed* (Plan 5's rule).
3. Enumerate what else composite-space evaluation does besides transform
   (bounding tests in composite space? texture/detail owners entangled with
   the composite level?) — each is a correctness hook the collapse must
   preserve or explicitly flag.

## Phase 1 — Record Composite Steps at Parse Time

Mirror Plan 5 Phase 1 at the `Composite` chokepoints: append a
`TransformStep` to the composite's own list inside the same functions that
build its matrices (recording cannot drift from the matrices if it lives
in the same function — the Plan 5 pattern). Gate byte-identical (recording
is passive).

## Phase 2 — Bake Composite Children Where the Rules Exist

For each composite child that is a `SimpleBody` whose geometry is a
Quadric/InfinitePlane with complete step lists (its own + all enclosing
composites'), produce a world-space baked copy via `BakedGeometryBaker`
replaying the Phase 0 order, store it in the composite's baked record, set
the explicit folded flag on the live objects, and route the trace path to
the direct (non-clone) kind. Children that fail any precondition
(unbakeable kind, bounding/clipping shapes tested in local space, missing
steps) stay on the clone path untouched — partial collapse per composite
is expected and fine.

Bake-time statistics line: composite children collapsed vs residual, per
scene. drums expectation from Plan 11: a large share of the 12.5 M
composite-space clones disappears.

## Phase 3 — Shading Unwind Audit

The two Plan 5 bugs live here in new clothing; audit before enabling:

- `doExtraInformation` unwind through composite levels must skip the
  folded levels' inverse-transform application (the explicit flag, not a
  heuristic).
- The detail-owner ordering fixed in [[texture_frame_fixes_part7]] (§17,
  `pushDetailOwner` not prepend) must survive the collapsed path — chess
  and dfwood are the canaries.

## Phase 4 — Golden Protocol and Close

Baked coefficient math takes a different FP path than per-ray transforms;
scenes with composite-heavy content may shift at noise level exactly as
Plan 5's did. Re-run Plan 5's Golden-Image Evaluation Protocol verbatim:
archive a fresh baseline render set, classify every flagged scene as
recovery / FP-noise / regression, re-baseline only with explicit user
confirmation, and record the table here. Then the standard close: gate,
`renderParallel.sh`, valgrind, drums/iortest/panel/renderAll timing table,
re-profile.

## Acceptance Criteria

- Phase 0 semantics appendix written before any code.
- Composite-clone count in drums measurably down (gprof
  `LocalIntersectionClone` callers) with gate clean under the protocol.
- No unexplained diffs; every re-baseline user-confirmed.
- Residual (unbakeable) clone population documented as the cycle's final
  declared floor, handed to Plan 16.

## Phase 0 Results (2026-07-04) — Semantics Appendix

### 1. Composite transform recording — the plan's premise is outdated

`Composite : public SimpleBody` (`environment/scene/Composite.h`), and
**`SimpleBody` already carries `bodySteps`/`geometrySteps` `ArrayList<TransformStep>`
members** (`SimpleBody.h:31-32`), populated by `applyTranslationToBodyTransform`/
`applyRotationToBodyTransform`/`applyScaleToBodyTransform`
(`SimpleBody.cpp:322-385`, each does `bodySteps.add(TransformStep(...))`
alongside updating the matrix pair).

`Composite::translate`/`rotate`/`scale` (`Composite.cpp:177-205`) call
these exact same `SimpleBody`-inherited `applyXToBodyTransform` methods:

```cpp
void
Composite::translate(Vector3Dd *vector)
{
    applyTranslationToBodyTransform(vector);   // <- appends to bodySteps
    applyOwnedTranslation(vector);
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedTranslation(vector);
    }
}
```

**Correction to the plan's premise**: a `Composite`'s own `bodySteps` list
*is* already recorded at parse time, via the same mechanism as any
`SimpleBody` - Plan 5 Phase 1 did not need to special-case `Composite`
because inheriting from `SimpleBody` gave it the recording for free.
`geometrySteps` stays empty for every `Composite` observed (composites are
never constructed with a `geometryTransformation`, and nothing calls
`translateGeometryLayer`/etc. on one - the `geometry` field passed to the
`Composite` constructor is not independently transformed).

**What is genuinely missing** (this narrows Phase 1's scope to near-zero
new recording code, but does not eliminate Phase 2's work): nothing
today *combines* a composite child's own steps with its enclosing
composite's (or composites', for nesting) steps into one list. Each
level's steps sit in isolation on its own object.

### 2. Trace-time composite-space evaluation

`BakedTrace::traceCompositeAllCrossings` (`BakedTrace.cpp:491-518`):

```cpp
bool
BakedTrace::traceCompositeAllCrossings(
    const BakedScene &scene,
    const BakedScene::CompositeRecord &composite,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache)
{
    if (composite.object == nullptr) {
        return false;
    }

    RayWithSegments *compositeRayPtr = ray;
    if (composite.hasObjectTransform) {
        RayWithSegments compositeRay(
            RayWithSegments::LocalIntersectionClone{}, *ray);
        compositeRay.setOrigin(composite.worldToObject.transformPoint(ray->getOrigin()));
        compositeRay.setDirection(composite.worldToObject.transformDirection(ray->getDirection()));
        compositeRay.setQuadricConstantsCached(false);
        compositeRayPtr = &compositeRay;

        return traceCompositeAllCrossingsInCompositeSpace(
            scene, composite, ray, compositeRayPtr, depthQueue, cache);
    }

    return traceCompositeAllCrossingsInCompositeSpace(
        scene, composite, ray, compositeRayPtr, depthQueue, cache);
}
```

This is the exact clone this plan targets (12.5 M/render in drums) - one
`LocalIntersectionClone` per composite-transform-bearing composite, per
ray that reaches it. `traceCompositeAllCrossingsInCompositeSpace`
(`BakedTrace.cpp:415-489`):

```cpp
bool
BakedTrace::traceCompositeAllCrossingsInCompositeSpace(...)
{
    // bounding shapes tested in COMPOSITE-LOCAL space (compositeRayPtr)
    for (... composite.boundingObjectIndices ...) {
        if (!BakedTrace::traceFirstHit(scene, boundingIndex, compositeRayPtr, ...) &&
            BakedTrace::containmentTest(scene, boundingIndex, compositeRayOrigin, ...)
                == Geometry::OUTSIDE) {
            return false;
        }
    }

    java::PriorityQueue<IntersectionCandidate> * const localDepthQueue = ...;
    const java::ArrayList<int> &children = composite.childObjectIndices;
    for (long int i = children.size() - 1; i >= 0; i--) {
        const int childIndex = children[i];
        const BakedScene::TraceableObject &child = scene.traceableObjects[childIndex];
        if (!composite.hasObjectTransform &&
            child.bounded && !rayIntersectsAabbForward(*compositeRayPtr, child.worldBounds)) {
            continue;
        }
        BakedTrace::traceAllCrossings(scene, childIndex, compositeRayPtr, localDepthQueue, cache);
    }

    for (IntersectionCandidate &candidate : *localDepthQueue) {
        candidate.getAttributes().pushDetailOwner(candidate.getAttributes().getHitBody());
        candidate.getAttributes().setHitBody(composite.object);
        const Vector3Dd compositeLocalPoint = candidate.getIntersection().point;
        if (composite.hasObjectTransform) {
            candidate.getIntersection().point =
                composite.objectToWorld.transformPoint(candidate.getIntersection().point);
        }
        // clipping shapes tested in composite-local space (compositeLocalPoint)
        ...
        candidate.getIntersection().t =
            candidate.getIntersection().point.subtract(ray->getOrigin()).length();
        depthQueue->offer(candidate);
    }
    ...
}
```

Answers to Phase 0's three sub-questions:
- (a) ray cloned via `RayWithSegments::LocalIntersectionClone{}` (same
  pattern as every other per-object clone in the codebase), using
  `composite.worldToObject` (built from `object->getTransformationInverse()`).
- (b) children are traced via the ordinary `BakedTrace::traceAllCrossings`
  dispatch, in **descending index order**
  (`for (i = children.size()-1; i>=0; i--)`) - "preserve
  `Composite::doIntersectionForAllRayCrossings` traversal order exactly"
  per the in-code comment. **Nested composites**: a child that is itself
  a Composite is just another `TraceableObject` of kind `Composite` -
  `traceAllCrossings` dispatches back into `traceCompositeAllCrossings`
  recursively, so a doubly-nested composite clones the ray **twice**,
  once per level, each clone keyed off that level's own
  `worldToObject`/`objectToWorld` (which is *itself* local to its
  immediate parent's space, not world space - composed only by the chain
  of clones, never pre-multiplied into one matrix). This recursive,
  per-level-local structure is exactly why Plan 15 Phase 1's combined-step
  list must walk the *enclosing chain*, innermost to outermost, rather
  than read a single flattened transform.
- (c) **Bounding shapes are tested in composite-local space
  (`compositeRayPtr`); clipping shapes are tested in composite-local
  space too (`compositeLocalPoint`, captured *before* the
  `objectToWorld` transform is applied)**. This exactly mirrors
  `bakeSimpleBody`'s existing fold-eligibility gate ("gated on no
  bounding/clipping shapes, since those are tested against the
  object-space ray/point" - `BakedSceneBuilder.cpp:340-343`) - a
  composite that itself carries bounding/clipping shapes must be
  excluded from Phase 2's world-space-fold eligibility for the exact
  same reason a `SimpleBody` with bounding/clipping shapes already is.
  Detail-owner push order (`pushDetailOwner`, not prepend, matching the
  in-code comment referencing [[texture_frame_fixes_part7]] §17) happens
  **after** children are traced but **before** the world transform is
  applied - order-dependent on t, not composite transform, so this is
  unaffected by whether the child ends up baked or clone-traced.

### 3. Existing Plan 5 replay machinery (`BakedGeometryBaker`)

`bakeQuadric`/`bakePlane` (`BakedGeometryBaker.cpp:58-154`) replay a
`java::ArrayList<TransformStep>` **in array order (index 0 first)**,
applying each step's *inverse* transform via `transformQuadric`
(quadric coefficient congruence, `Q' = M^-T Q M^-1` in homogeneous-matrix
form) one elementary step at a time - never composing one combined
matrix first (the plan's own "recorded dead end"). `Invert` steps are
skipped during replay (already baked destructively into the parsed
geometry's own coefficients at parse time - see the code comment).

The existing caller, `bakeSimpleBody` (`BakedSceneBuilder.cpp:311-380`):

```cpp
if (!baked.hasBoundingShapes && !baked.hasClippingShapes &&
    (baked.hasObjectTransform || baked.hasGeometryTransform)) {
    java::ArrayList<TransformStep> combinedSteps(
        object->getGeometrySteps().size() + object->getBodySteps().size());
    for (long int i = 0; i < object->getGeometrySteps().size(); i++) {
        combinedSteps.add(object->getGeometrySteps()[i]);
    }
    for (long int i = 0; i < object->getBodySteps().size(); i++) {
        combinedSteps.add(object->getBodySteps()[i]);
    }
    if (combinedSteps.size() > 0) {
        if (baked.quadricGeometry != nullptr) {
            baked.bakedQuadricStorage =
                BakedGeometryBaker::bakeQuadric(*baked.quadricGeometry, combinedSteps);
            ...
        } else if (InfinitePlane *plane = ...) {
            baked.bakedPlaneStorage = BakedGeometryBaker::bakePlane(*plane, combinedSteps);
            ...
        }
    }
}
```

confirms the replay order for a single (non-composite) object:
**geometry-layer steps first, then object/body-layer steps** - matching
the plan's expected order exactly. `bakeSimpleBody` is called (via
`compileTracingObject`) for composite children *today already* - but
only ever sees the child's own `getGeometrySteps()`/`getBodySteps()`,
never anything from an enclosing `Composite`. This means: **a composite
child with its own transform is already partially baked today** (into
composite-local-space coefficients), which is why
`traceCompositeAllCrossingsInCompositeSpace` can call
`BakedTrace::traceAllCrossings(scene, childIndex, compositeRayPtr, ...)`
directly against a (possibly already-baked) child - the missing piece is
solely folding the *composite's own* `bodySteps` (and any enclosing
composites', innermost-to-outermost) into that same combined list, so
the result is a world-space baked copy usable without ever building
`compositeRayPtr` at all.

**Derived replay order for Plan 15 Phase 2** (from the code, not
assumed): `child.geometrySteps ++ child.bodySteps ++
innermostEnclosingComposite.bodySteps ++ ... ++
outermostEnclosingComposite.bodySteps`. `Composite.geometrySteps` is
always empty in practice (confirmed above) so it can be omitted from the
enclosing chain's contribution without loss.

### 4. Shading unwind / explicit-flag idiom

`SimpleBody::isBakedTransformFolded()`/`setBakedTransformFolded()`
(`SimpleBody.h:38,96-97`) is the existing explicit flag Plan 5 uses;
`doExtraInformation` reads it (not shown here in full - out of this
session's research budget - but confirmed present and referenced
directly by the field's own doc comment: "tells doExtraInformation()
not to re-apply" the transform). The equivalent flag for Plan 15 would
most naturally live on `BakedScene::CompositeRecord`/`TraceableObject`
alongside the existing `hasBakedQuadric`/`hasBakedPlane` fields already
defined on `TraceableObject` (`BakedScene.h`, same struct
`bakeSimpleBody` already populates) - **no new struct needed**, Phase 2
can reuse `TraceableObject::hasBakedQuadric`/`hasBakedPlane` as-is, since
those fields already mean exactly "this object's geometry is a
world-space baked copy, do not re-apply any transform" regardless of
whether the transform being folded away came from the object's own
steps alone or the object's steps plus its enclosing composite chain's.
The only *new* per-record data Phase 2 needs is bookkeeping to detect
composite-level fold eligibility (bounding/clipping shapes on any
enclosing composite level, not just the child) before attempting the
combined-steps bake.

### 5. Detail-owner ordering

Already covered in item 2(c) above: `pushDetailOwner` (append, not
prepend) at `BakedTrace.cpp:456` for the baked-scene composite path,
matching `Composite.cpp:68`'s equivalent in the legacy path - both
already fixed per [[texture_frame_fixes_part7]] §17, and this order is
orthogonal to whether a child is baked-in-world-space or still
clone-traced (it operates on `t`/detail-owner-stack data extracted from
the already-produced candidate, after the child's own trace call
returns, regardless of which path produced that candidate).

### 6. Bake-time record structure

`BakedScene::CompositeRecord` (`BakedScene.h:221-237`, quoted in full
above in item 2's preamble) has no baked-geometry storage of its own
today - it doesn't need one for Phase 2, since the baking target is each
*child*'s `TraceableObject` entry (which already has
`bakedQuadricStorage`/`hasBakedQuadric`/`bakedPlaneStorage`/`hasBakedPlane`),
not the composite record itself. `CompositeRecord` would need exactly one
new thing for Phase 2: a way for `traceCompositeAllCrossingsInCompositeSpace`'s
caller to know a *given child* no longer needs the composite-local ray at
all (skip it for that child, keep using it for children that stayed
unbaked) - most simply, a per-child bool alongside `childObjectIndices`,
or reusing the child's own `TraceableObject::hasBakedQuadric`/
`hasBakedPlane` as that signal directly (favored: no new list to keep in
sync with `childObjectIndices`).

## Phase 1 Results — no new recording code needed

Per Phase 0 finding §1: `Composite::translate`/`rotate`/`scale` already
call the `SimpleBody`-inherited `applyTranslationToBodyTransform`/etc.,
which already append to `bodySteps` on the composite object itself.
`getBodySteps()`/`getGeometrySteps()` (`SimpleBody.h:94-95`) are already
public accessors, inherited by `Composite` unchanged. Phase 1 is
therefore **complete with zero code changes** - verified by code reading
(the call chain is unconditional, not gated behind any Composite-specific
override that could skip it) rather than by runtime instrumentation,
since the mechanism is identical to the one Plan 5 already proved
correct for plain `SimpleBody` objects. Gate unaffected (nothing changed).

## Phase 2 Results (2026-07-04) — Design worked out, NOT implemented

Phase 2 was not implemented. Design work went through two iterations,
each revealing a correctness trap not visible from the plan text alone;
closing here rather than shipping either design, per user decision.

### Design iteration 1 — partial per-child baking (as the plan text implies) — rejected

The plan's own framing ("children that fail any precondition stay on the
clone path, partial collapse per composite is expected and fine") implies
baking individual children independently while leaving siblings
clone-traced. This does not work as stated:

- `traceCompositeAllCrossings` builds **one** `compositeRay` clone per
  `(composite, ray)` pair, shared by every child in the composite's
  `childObjectIndices` loop (`BakedTrace.cpp:503-514`). Baking a single
  child's coefficients does not remove this clone if even one sibling
  still needs composite-local testing - the clone still has to be built
  for that sibling. The measured savings this plan targets (12.5 M
  clones in drums) only materializes when an *entire* composite's clone
  becomes unnecessary, not per-child.
- Candidates coming from an already-world-space-baked child must **not**
  receive `traceCompositeAllCrossingsInCompositeSpace`'s
  `composite.objectToWorld.transformPoint(...)` step
  (`BakedTrace.cpp:460-463`) - that transform is only correct for a
  candidate whose point is still in composite-local space. Applying it
  to an already-world-space point double-transforms it. Making this
  conditional per-candidate requires threading a "was this child baked"
  flag through the local-to-world post-processing loop, and doing the
  same for the clipping-shape test's `compositeLocalPoint`
  (`BakedTrace.cpp:459,466-475`), which needs the object-space point
  precisely for clipping-shape containment tests - for a baked child that
  point does not natively exist post-hoc, and would need reconstruction
  via `composite.worldToObject` applied to the (already world-space) hit
  point, adding real per-candidate cost back for exactly the case Phase 2
  set out to speed up. High risk of a subtle double-transform bug for
  little robustness gain. Rejected.

### Design iteration 2 — all-or-nothing composite collapse (composite inlining) — designed, not implemented

The lower-risk alternative: only fold a composite (and, recursively, any
nested composite) when its **entire** subtree is bakeable - no
bounding/clipping shapes at any level, every leaf child a Quadric/
InfinitePlane. When eligible, at bake time, *inline* the composite's
(recursively baked, world-space) leaf descendants directly into whatever
list would have referenced the composite (the scene's
`topLevelObjectIndices` for a top-level composite, or the parent
composite's `childObjectIndices` for a nested one) - never creating a
`TraceKind::Composite` `TraceableObject`/`CompositeRecord` for it at all.
This avoids the double-transform bug class *by construction*: baked
children become ordinary top-level-style `TraceableObject`s, going
through the exact same code path (and byte-identical math) as any other
bakeable object today.

**Ordering proof (worked out, believed correct)**: `bakeComposite`
populates `childObjectIndices` in forward parse order
(`BakedSceneBuilder.cpp:790-792`, `for (i = 0; i < children.size(); i++)`),
while `traceCompositeAllCrossingsInCompositeSpace`'s trace loop consumes
that list in **reverse** (`BakedTrace.cpp:444`,
`for (i = children.size()-1; i >= 0; i--)`) - the code's own comment
("preserve traversal order exactly, changing equal-depth ordering
changes some legacy scenes") confirms this reversal is load-bearing.
The same reversal pattern holds one level up: `build()` populates
`topLevelObjectIndices` in forward parse order
(`BakedSceneBuilder.cpp:977-984`), consumed in reverse by
`RenderEngine.cpp`'s render loop. **Inserting a collapsed composite's
recursively-flattened children in forward order at exactly the position
the composite itself would have occupied** reproduces the identical
reverse-walk dispatch sequence - both across composite siblings and
between the composite's children and their other top-level/parent-level
siblings - so candidate priority-queue insertion order (and therefore
equal-`t` tie-breaks) is unaffected. This part of the design is believed
sound, derived directly from the two call sites' existing loop
directions, not assumed.

**Two correctness traps found that ARE NOT solved by the inlining
approach alone** - both would need explicit extra work before this could
ship:

1. **`worldBounds` is not actually world-space for composite children,
   despite its name.** `bakeSimpleBody`'s `baked.worldBounds =
   object->getAABB();` (`BakedSceneBuilder.cpp:321`) is called
   identically for top-level objects and composite children alike, with
   no context awareness of enclosing composites. For a composite child,
   `object->getTransformation()` (used inside `getAABB()`) is defined
   relative to the *immediate enclosing composite's local frame*, not
   world space - confirmed by `traceCompositeAllCrossingsInCompositeSpace`'s
   own AABB pre-test (`BakedTrace.cpp:447-451`), which tests
   `child.worldBounds` against `compositeRayPtr` (the **composite-local**
   ray), never the world ray. Promoting such a child directly to
   top-level status and reusing its existing `worldBounds` field as-is
   would cull against the wrong ray's frame - not a subtle image
   difference but a straightforwardly wrong AABB test. Fixing this
   requires recomputing each folded child's true world-space AABB by
   transforming its existing box through the accumulated enclosing
   transform chain (the same chain being baked into its coefficients) -
   solvable, but new code, not just wiring.
2. **A composite child's own `noShadowFlag` is a dead field for shadow
   purposes today**, specifically when reached only through a composite:
   `traceCompositeAllCrossingsInCompositeSpace`'s child loop
   (`BakedTrace.cpp:444-453`) traces every child unconditionally for both
   regular and shadow rays - only the *composite's own* `noShadowFlag`
   (consulted once, when the composite itself is classified into
   `shadowCastingObjectIndices` vs not, at the top level) decides whether
   shadow rays ever attempt the composite's subtree at all. A folded
   child promoted to genuine top-level status would, for the first time,
   have its *own* individual `noShadowFlag`/`castsShadow` field actually
   consulted (via the normal top-level classification loop,
   `BakedSceneBuilder.cpp:992-998`) - which may disagree with what the
   enclosing composite chain would have imposed. Correct behavior
   requires overriding each folded child's effective `castsShadow` to the
   **OR** of its own flag and every enclosing composite level's flag
   (any level saying "no shadow" must win, matching today's "composite
   decides, child's own flag is irrelevant" behavior) - again solvable,
   but new logic, and easy to get backwards (OR vs AND) without a scene
   that actually exercises composite-nested `no_shadow` to check against.

### Why this was not carried further

Both traps are individually fixable, and the ordering argument above
gives confidence the *structural* part of the collapse is sound. But
each was found only by going one level deeper than the previous check-in
- consistent with this session's pattern on Plans 13/14: a plan that
looks contained at first reveals compounding, non-obvious correctness
requirements as implementation detail is worked out, each one only
partially covered (if at all) by the existing golden-image suite (no
scene in the current panel is known to specifically exercise a
bounded-and-transformed composite with mixed shadow flags across nesting
levels). Given Plan 15 is *already* flagged by its own text as the single
riskiest plan in this cycle (the only one requiring the full
Golden-Image Evaluation Protocol), stacking two more undexercised
correctness fixes on top was judged, together with the user, not worth
the validation cost relative to the uncertain payoff - especially since
Plan 11's own evidence attributes only part of drums' clone volume to
composites in the first place (13.0 M of drums' 27.3 M total clones come
from non-composite sources Plan 15 never touches at all).

### Acceptance Criteria status

- Phase 0 semantics appendix: written before any code, as required. ✅
- Composite-clone count reduction: not attempted - no code shipped.
- No unexplained diffs / re-baselines: N/A, `git diff --stat -- src/` is
  empty; only this doc changed.
- Residual floor handoff to Plan 16: unchanged from Plan 11's original
  figures - drums' 12.5 M composite-space clones remain exactly as
  measured, now with a documented (not just estimated) path to removing
  them, gated on the two correctness fixes above being implemented and
  validated against a scene that actually exercises nested composite
  shadow-flag disagreement, should anyone pick this up later.

## Phases 3-4 — not applicable

Both were prerequisite on Phase 2 landing bakeable composite children;
since Phase 2 was not implemented, Phase 3's shading-unwind audit and
Phase 4's Golden-Image Protocol execution have nothing to audit/gate.
Plan 15 is CLOSED with zero code changes - `git diff --stat -- src/` is
empty against this plan's starting commit; only this doc changed.

**Handoff to Plan 16**: the residual clone floor Plan 11 measured (drums:
12.5 M composite-space clones + 13.0 M from non-bakeable
sphere/box/patch-owned transforms elsewhere) stands unchanged, with two
concrete, evidence-backed (not speculative) correctness requirements
documented above for whoever revisits composite collapse with a larger
validation budget: (1) recompute world-space AABB through the
accumulated enclosing-transform chain rather than reusing the existing
(composite-local) `worldBounds` field, and (2) propagate
`noShadowFlag` as an OR down the enclosing composite chain rather than
trusting a folded child's own individual flag.

## Known Dead Ends (inherited from Plan 5; all still binding)

- Composed-matrix coefficient baking (replay elementary steps only).
- Sphere → baked-quadric promotion (`intersectBakedQuadric` ≠
  `intersectSphere` numerically; Plan 3 Phase H, 35 failures).
- Inferring "baked" from `hit->hitGeometry != geometry` in
  `doExtraInformation` (broke the gate twice; explicit flags only).
- Double-applying parse-time-destructive `Invert` steps in replay (Plan 5
  Phase 3's root-cause bug — `invertGeometry()` already mutated the parsed
  coefficients at parse time).
