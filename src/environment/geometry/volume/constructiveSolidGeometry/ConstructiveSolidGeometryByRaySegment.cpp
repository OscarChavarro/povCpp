#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "common/Config.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.h"
#include "environment/geometry/surface/InfinitePlane.h"

// A top-level union of unbounded planes has no real in/out solid to
// classify (Table 3 of [ROTH1982].3.3 assumes bounded operands), so
// allIntersections() skips the merge for it below.
static bool
isUnionOfBarePlanes(BooleanSetOperations geometryType, java::ArrayList<SimpleBody*> &children)
{
    if (geometryType == BooleanSetOperations::DIFFERENCE ||
        geometryType == BooleanSetOperations::INTERSECTION) {
        return false;
    }
    for (long int i = 0; i < children.size(); i++) {
        if (dynamic_cast<InfinitePlane *>(children[i]->getWrappedGeometry()) == nullptr) {
            return false;
        }
    }
    return true;
}

ConstructiveSolidGeometryByRaySegment::ConstructiveSolidGeometryByRaySegment(BooleanSetOperations initialGeometryType) :
    ConstructiveSolidGeometry(initialGeometryType)
{
}

ConstructiveSolidGeometryByRaySegment::ConstructiveSolidGeometryByRaySegment(const ConstructiveSolidGeometryByRaySegment &other) :
    ConstructiveSolidGeometry(other.getGeometryType()),
    topLevel(other.isTopLevel())
{
    for (long int i = 0; i < other.getShapes().size(); i++) {
        getShapes().add((SimpleBody *)other.getShapes()[i]->copy());
    }
}

void *
ConstructiveSolidGeometryByRaySegment::copy()
{
    return new ConstructiveSolidGeometryByRaySegment(*this);
}

// Per-child RAYCAST classification ([ROTH1982].3.3, "In-Out Classification").
RaySegments
ConstructiveSolidGeometryByRaySegment::buildRaySegments(RayWithSegments *ray, SimpleBody *child)
{
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        ray->getIntersectionQueuePool()->pop(128);

    child->allIntersections(ray, localDepthQueue);

    // One containment sample per child suffices: later crossings just
    // toggle it ([ROTH1982].3.3 - a ray alternates in/out at each surface
    // crossing).
    bool initialInside;
    if (localDepthQueue->size() > 0) {
        IntersectionCandidate firstCandidate = localDepthQueue->peek();
        Vector3Dd samplePoint = ray->getOrigin().add(
            ray->getDirection().multiply(0.5 * firstCandidate.getIntersection().t));
        initialInside =
            child->doContainmentTest(samplePoint, 0.0) == Geometry::INSIDE;
    } else {
        // See [ROTH1982] Bug B (math/trough.pov): a continuation ray
        // starting exactly on this child's surface needs a positive offset,
        // not a negative tolerance, to land reliably on the correct side.
        Vector3Dd samplePoint =
            ray->getOrigin().add(ray->getDirection().multiply(2.0 * Config::SMALL_TOLERANCE));
        initialInside =
            child->doContainmentTest(samplePoint, 0.0) == Geometry::INSIDE;
    }

    java::ArrayList<RaySegmentCrossing> crossings{localDepthQueue->size()};
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

// Merge/classify/simplify, the three-step process of [ROTH1982].3.3
// ("Combining Left and Right Classifications", Figs. 8-9, Table 3).
RaySegments
ConstructiveSolidGeometryByRaySegment::mergeByMembership(
    const RaySegments &left,
    const RaySegments &right,
    bool (*combine)(bool insideLeft, bool insideRight))
{
    const java::ArrayList<RaySegmentCrossing> &leftCrossings = left.getCrossings();
    const java::ArrayList<RaySegmentCrossing> &rightCrossings = right.getCrossings();

    java::ArrayList<RaySegmentCrossing> outCrossings{leftCrossings.size() + rightCrossings.size()};
    bool insideLeft = left.isInitialInside();
    bool insideRight = right.isInitialInside();
    const bool initialCombined = combine(insideLeft, insideRight);

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
ConstructiveSolidGeometryByRaySegment::mergeUnion(const RaySegments &left, const RaySegments &right)
{
    return mergeByMembership(left, right, combineUnion);
}

RaySegments
ConstructiveSolidGeometryByRaySegment::mergeIntersection(const RaySegments &left, const RaySegments &right)
{
    return mergeByMembership(left, right, combineIntersection);
}

RaySegments
ConstructiveSolidGeometryByRaySegment::mergeDifference(const RaySegments &left, const RaySegments &right)
{
    return mergeByMembership(left, right, combineDifference);
}

// RAYCAST descends the tree and combines left/right classifications on the
// way back up ([ROTH1982].3.3), folded here over an n-ary children list.
int
ConstructiveSolidGeometryByRaySegment::allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    java::ArrayList<SimpleBody*> &children = getShapes();
    if (children.size() == 0) {
        return false;
    }

    if (isTopLevel() && isUnionOfBarePlanes(getGeometryType(), children)) {
        bool anyFound = false;
        for (long int i = 0; i < children.size(); i++) {
            if (children[i]->allIntersections(ray, depthQueue)) {
                anyFound = true;
            }
        }
        return anyFound;
    }

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
ConstructiveSolidGeometryByRaySegment::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    java::ArrayList<SimpleBody*> &children = getShapes();
    if (children.size() == 0) {
        return OUTSIDE;
    }

    bool isInside;
    switch (getGeometryType()) {
    case BooleanSetOperations::DIFFERENCE:
        // Table 3's "-" row in [ROTH1982].3.3.
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

// De Morgan complement of Table 3's three operators ([ROTH1982].3.3).
void
ConstructiveSolidGeometryByRaySegment::invertGeometry()
{
    java::ArrayList<SimpleBody*> &children = getShapes();

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
        setGeometryType(BooleanSetOperations::UNION);
        if (children.size() > 0) {
            children[0]->invert();
        }
    }
}
