#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "common/Config.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/compound/CSGByRaySegment.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/scene/SimpleBody.h"

/**
 * True when every child of this node is a bare InfinitePlane: POV's classic
 * "two (or more) infinite half-space planes unioned into a mirror corner"
 * idiom (e.g. skyvase.pov's reflective backdrop). Each plane is unbounded,
 * so a child that the ray never crosses forward gets sampled by
 * buildRaySegments as "inside" for the *entire* ray (most of space, for an
 * unbounded half-space) rather than "outside" - and folded through
 * combineUnion's OR, that phantom inside-ness permanently swallows every
 * sibling's real crossing (Table 3's "+" row never sees a False on this
 * side to toggle against), making the whole union invisible from then on.
 * That is exactly the missing-mirror-reflection bug: a ray reflected off a
 * separate object straight at this union's own surface can land on the
 * dominated side and find nothing, even though the union is plainly visible
 * head-on. Legacy point-membership CSG never hits this, because
 * CSG::allCsgUnionIntersections offers every child's own raw crossings
 * unfiltered, regardless of the other children - there is no "interior of a
 * sibling" concept for a union of bare half-spaces in the first place, only
 * for genuinely bounded solids. allIntersections() below uses this together
 * with isTopLevel() to gate skipping the merge entirely in favour of that
 * same per-child, sibling-blind concatenation - but ONLY when this node is
 * the outermost CSG for its object (set by CsgParser, never for a union
 * nested inside another CSG block's body). A *nested* union of bare planes
 * (e.g. chess.pov's plane-unions carving notches inside an outer
 * intersection/difference) keeps the strict merge unconditionally, because
 * its parent genuinely depends on a clean, correctly-alternating in/out
 * classification to combine with - exactly what the merge provides and raw
 * concatenation does not. Never applies to INTERSECTION/DIFFERENCE either,
 * where clipping a bounded solid against an unbounded half-space sibling
 * (Q3) genuinely needs the real sampled classification.
 */
static bool
isUnionOfBarePlanes(BooleanSetOperations geometryType, java::ArrayList<TransformableElement*> &children)
{
    if (geometryType == BooleanSetOperations::DIFFERENCE ||
        geometryType == BooleanSetOperations::INTERSECTION) {
        return false;
    }
    for (long int i = 0; i < children.size(); i++) {
        SimpleBody *body = dynamic_cast<SimpleBody *>(children[i]);
        if (body == nullptr || dynamic_cast<InfinitePlane *>(body->getGeometry()) == nullptr) {
            return false;
        }
    }
    return true;
}

CSGByRaySegment::CSGByRaySegment(BooleanSetOperations initialGeometryType) :
    CSG(initialGeometryType)
{
}

CSGByRaySegment::CSGByRaySegment(const CSGByRaySegment &other) :
    CSG(other.getGeometryType()),
    topLevel(other.isTopLevel())
{
    // CSG's own copy constructor copies children back-to-front (harmless
    // there, since CSG::allCsgUnionIntersections/allCsgIntersectIntersections
    // are order-independent). DIFFERENCE under Roth is NOT order-independent
    // (children[0] is the minuend, Q2), so CSGByRaySegment needs its own
    // copy constructor that preserves parse order. Without this, every copy
    // of a CSGByRaySegment DIFFERENCE (e.g. a #declare'd object referenced
    // elsewhere via `object { Name }`) would silently subtract the wrong
    // operand.
    for (long int i = 0; i < other.getShapes().size(); i++) {
        getShapes().add((TransformableElement *)other.getShapes()[i]->copy());
    }
}

void *
CSGByRaySegment::copy()
{
    return new CSGByRaySegment(*this);
}

/**
 * Classifies a single child solid along the ray: [ROTH1982].3.3
 * ("In-Out Classification"), the "Intersecting Rays with Primitives" part.
 * Roth's RAYCAST returns the enter-exit ray parameters t[i] and surface
 * pointers S[i] for one primitive; here the equivalent per-child crossing
 * list is built directly from the child's own allIntersections(), which
 * already recurses through any sub-tree the child happens to be.
 */
RaySegments
CSGByRaySegment::buildRaySegments(RayWithSegments *ray, TransformableElement *child)
{
    java::ArrayList<RaySegmentCrossing> crossings{8};

    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        ray->getIntersectionQueuePool()->pop(128);

    child->allIntersections(ray, localDepthQueue);

    // Only ONE containment sample is needed per child: the state just before
    // the ray's first crossing (or, if there are no crossings at all, the
    // state along the whole ray). Every later crossing simply toggles that
    // state ([ROTH1982].3.3, "Intersecting Rays with Primitives": a ray
    // alternates in/out at each successive surface crossing). Sampling
    // doContainmentTest() independently AT EVERY crossing (an earlier version
    // of this method did that) is both unnecessary and fragile: it relies on
    // a fixed epsilon offset clearing whatever surface comes next, which
    // breaks down whenever two crossings of the same child end up closer
    // together than that epsilon (e.g. near a tangential/grazing crossing of
    // an open quartic, or where two cutting planes meet) - sampling there can
    // land back on the wrong side of the very next crossing.
    // localDepthQueue is a min-heap by t: poll() drains it in ascending order,
    // so the first poll() is always the ray's first crossing of this child.
    bool initialInside;
    if (localDepthQueue->size() > 0) {
        IntersectionCandidate firstCandidate = localDepthQueue->peek();
        Vector3Dd samplePoint = ray->getOrigin().add(
            ray->getDirection().multiply(0.5 * firstCandidate.getIntersection().t));
        initialInside =
            child->doContainmentTest(samplePoint, 0.0) == TransformableElement::INSIDE;
    } else {
        // No crossings at all: either the ray misses the child entirely, or
        // the only crossing was degenerate and got filtered out by the
        // child's own intersection routine for being within its tolerance
        // window of t=0 (e.g. a shadow ray leaving a surface it stands on,
        // or a half-space whose plane happens to pass near the ray's
        // origin). Sampling containment exactly at the origin would be
        // unreliable in that degenerate case (it sits right on the boundary
        // the child already decided to ignore), so the sample point is
        // pushed just past the same tolerance window the leaf shapes use,
        // landing solidly on whichever side the ray actually continues on.
        // Zero (not negative) tolerance for the same reason as the
        // has-crossings branch above: a negative tolerance of the same order
        // as the offset distorts the test into requiring dir.normal <=
        // -tolerance/offset, which misclassifies this child as OUTSIDE for
        // any but a near-head-on approach - exactly what happens to a
        // secondary/continuation ray spawned from a point that sits right on
        // this child's own surface (e.g. a transparency ray continuing past
        // a `texture { color Clear }` clipping plane: see [ROTH1982] Bug B,
        // math/trough.pov).
        Vector3Dd samplePoint =
            ray->getOrigin().add(ray->getDirection().multiply(2.0 * Config::SMALL_TOLERANCE));
        initialInside =
            child->doContainmentTest(samplePoint, 0.0) == TransformableElement::INSIDE;
    }

    bool currentlyInside = initialInside;
    while (localDepthQueue->size() > 0) {
        IntersectionCandidate candidate = localDepthQueue->poll();
        currentlyInside = !currentlyInside;
        crossings.add(RaySegmentCrossing(candidate.getIntersection().t, currentlyInside, candidate));
    }

    ray->getIntersectionQueuePool()->push(localDepthQueue);
    return RaySegments(crossings, initialInside);
}

static bool
combineUnion(bool insideLeft, bool insideRight)
{
    return insideLeft || insideRight;
}

static bool
combineIntersection(bool insideLeft, bool insideRight)
{
    return insideLeft && insideRight;
}

static bool
combineDifference(bool insideLeft, bool insideRight)
{
    return insideLeft && !insideRight;
}

/**
 * Combines two children's classifications into one, by the three-step
 * process of [ROTH1982].3.3 ("Combining Left and Right Classifications"):
 * (1) merge the left and right crossings in sorted t order (Fig. 8's
 * "segmented composite ray"); (2) classify each resulting segment in/out
 * per the boolean operator and the running left/right in/out state
 * (Table 3, "Boolean Operations"); (3) simplify by emitting a crossing only
 * where the combined classification actually changes, which is the
 * ray-segment equivalent of merging contiguous same-classification segments
 * (Fig. 9, step 3).
 */
RaySegments
CSGByRaySegment::mergeByMembership(
    const RaySegments &left,
    const RaySegments &right,
    bool (*combine)(bool insideLeft, bool insideRight))
{
    java::ArrayList<RaySegmentCrossing> outCrossings{8};
    bool insideLeft = left.isInitialInside();
    bool insideRight = right.isInitialInside();
    const bool initialCombined = combine(insideLeft, insideRight);

    const java::ArrayList<RaySegmentCrossing> &leftCrossings = left.getCrossings();
    const java::ArrayList<RaySegmentCrossing> &rightCrossings = right.getCrossings();

    bool previousCombined = initialCombined;
    long int i = 0;
    long int j = 0;
    while ((i < leftCrossings.size()) || (j < rightCrossings.size())) {
        bool takeLeft;
        if (j >= rightCrossings.size()) {
            takeLeft = true;
        } else if (i >= leftCrossings.size()) {
            takeLeft = false;
        } else {
            takeLeft = leftCrossings[i].getT() <= rightCrossings[j].getT();
        }

        RaySegmentCrossing event;
        if (takeLeft) {
            event = leftCrossings[i];
            insideLeft = event.isEntering();
            i++;
        } else {
            event = rightCrossings[j];
            insideRight = event.isEntering();
            j++;
        }

        const bool newCombined = combine(insideLeft, insideRight);
        if (newCombined != previousCombined) {
            outCrossings.add(RaySegmentCrossing(event.getT(), newCombined, event.getHit()));
            previousCombined = newCombined;
        }
    }
    return RaySegments(outCrossings, initialCombined);
}

RaySegments
CSGByRaySegment::mergeUnion(const RaySegments &left, const RaySegments &right)
{
    return mergeByMembership(left, right, combineUnion);
}

RaySegments
CSGByRaySegment::mergeIntersection(const RaySegments &left, const RaySegments &right)
{
    return mergeByMembership(left, right, combineIntersection);
}

RaySegments
CSGByRaySegment::mergeDifference(const RaySegments &left, const RaySegments &right)
{
    return mergeByMembership(left, right, combineDifference);
}

/**
 * RAYCAST descends the composition tree, classifies the ray against each
 * primitive, then "returns up the tree combining the classifications of the
 * left and right subtrees" ([ROTH1982].3.3). This folds that same
 * recursive combine over an n-ary children list instead of Roth's strictly
 * binary L-SOLID-PTR/R-SOLID-PTR tree.
 */
int
CSGByRaySegment::allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    java::ArrayList<TransformableElement*> &children = getShapes();
    if (children.size() == 0) {
        return false;
    }

    // See isUnionOfBarePlanes's doc comment and CSGByRaySegment::topLevel:
    // a *top-level* union of bare half-space planes has no real "boolean
    // solid" semantics to classify in the first place (nothing above it
    // depends on its in/out state), so skip the merge below entirely and
    // match legacy CSG::allCsgUnionIntersections exactly - every child's own
    // raw crossings, unfiltered.
    if (isTopLevel() && isUnionOfBarePlanes(getGeometryType(), children)) {
        bool anyFound = false;
        for (long int i = 0; i < children.size(); i++) {
            if (children[i]->allIntersections(ray, depthQueue)) {
                anyFound = true;
            }
        }
        return anyFound;
    }

    // children[0] is the first child parsed (the minuend for DIFFERENCE,
    // Q2); union/intersection are commutative so the fold order does not
    // matter for them.
    RaySegments result = buildRaySegments(ray, children[0]);
    for (long int i = 1; i < children.size(); i++) {
        const RaySegments childSegments = buildRaySegments(ray, children[i]);
        switch (getGeometryType()) {
        case BooleanSetOperations::DIFFERENCE:
            result = mergeDifference(result, childSegments);
            break;
        case BooleanSetOperations::INTERSECTION:
            result = mergeIntersection(result, childSegments);
            break;
        default:
            result = mergeUnion(result, childSegments);
            break;
        }
    }

    bool intersectionFound = false;
    const java::ArrayList<RaySegmentCrossing> &resultCrossings = result.getCrossings();
    for (long int i = 0; i < resultCrossings.size(); i++) {
        depthQueue->offer(resultCrossings[i].getHit());
        intersectionFound = true;
    }
    return intersectionFound;
}

int
CSGByRaySegment::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    java::ArrayList<TransformableElement*> &children = getShapes();
    if (children.size() == 0) {
        return OUTSIDE;
    }

    bool isInside;
    switch (getGeometryType()) {
    case BooleanSetOperations::DIFFERENCE:
        // A0 \ (A1 u A2 u ...): inside the minuend and outside every
        // subtrahend (mirrors mergeDifference's combine rule, Table 3's "-"
        // row in [ROTH1982].3.3).
        isInside = children[0]->doContainmentTest(point, distanceTolerance) != OUTSIDE;
        for (long int i = 1; isInside && (i < children.size()); i++) {
            if (children[i]->doContainmentTest(point, distanceTolerance) != OUTSIDE) {
                isInside = false;
            }
        }
        break;

    case BooleanSetOperations::INTERSECTION:
        isInside = true;
        for (long int i = 0; isInside && (i < children.size()); i++) {
            if (children[i]->doContainmentTest(point, distanceTolerance) == OUTSIDE) {
                isInside = false;
            }
        }
        break;

    default:
        isInside = false;
        for (long int i = 0; !isInside && (i < children.size()); i++) {
            if (children[i]->doContainmentTest(point, distanceTolerance) != OUTSIDE) {
                isInside = true;
            }
        }
        break;
    }
    return isInside ? INSIDE : OUTSIDE;
}

/**
 * De Morgan complement of the three operators in Table 3 of [ROTH1982].3.3:
 * union and intersection swap (the usual complement rule), and since
 * difference is itself "intersection with a complement" in Roth's
 * formulation (A0 n ~A1 n ~A2 ...), only its minuend flips and the operator
 * becomes union.
 */
void
CSGByRaySegment::invertGeometry()
{
    java::ArrayList<TransformableElement*> &children = getShapes();

    if (getGeometryType() == BooleanSetOperations::INTERSECTION) {
        setGeometryType(BooleanSetOperations::UNION);
        for (long int i = children.size() - 1; i >= 0; i--) {
            children[i]->invert();
        }
    } else if (getGeometryType() == BooleanSetOperations::UNION) {
        setGeometryType(BooleanSetOperations::INTERSECTION);
        for (long int i = children.size() - 1; i >= 0; i--) {
            children[i]->invert();
        }
    } else {
        // DIFFERENCE is A0 \ (A1 u A2 u ...), i.e. A0 n ~A1 n ~A2 n ...; its
        // De Morgan complement is ~A0 u A1 u A2 u ... - only the minuend
        // flips, and the operator becomes UNION.
        setGeometryType(BooleanSetOperations::UNION);
        if (children.size() > 0) {
            children[0]->invert();
        }
    }
}
