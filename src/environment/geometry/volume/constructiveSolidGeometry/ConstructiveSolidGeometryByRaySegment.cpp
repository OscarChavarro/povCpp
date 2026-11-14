#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.h"
#include "environment/geometry/surface/RayCastingInfinitePlane.h"
#include "vsdk/toolkit/environment/scene/csg/CsgMembershipRule.h"
#include "vsdk/toolkit/environment/scene/csg/CsgSegmentClassification.txx"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

// A top-level union of unbounded planes has no real in/out solid to
// classify (Table 3 of [ROTH1982].3.3 assumes bounded operands), so
// doIntersectionForAllRayCrossings() skips the merge for it below.
static bool
isUnionOfBarePlanes(CsgOperation geometryType, const java::ArrayList<CsgOperand*> &children)
{
    if (geometryType == CsgOperation::DIFFERENCE ||
        geometryType == CsgOperation::INTERSECTION) {
        return false;
    }
    for (long int i = 0; i < children.size(); i++) {
        if (dynamic_cast<RayCastingInfinitePlane *>(children[i]->getGeometry()) == nullptr) {
            return false;
        }
    }
    return true;
}

ConstructiveSolidGeometryByRaySegment::ConstructiveSolidGeometryByRaySegment(CsgOperation initialGeometryType) :
    ConstructiveSolidGeometry(initialGeometryType)
{
}

ConstructiveSolidGeometryByRaySegment::ConstructiveSolidGeometryByRaySegment(const ConstructiveSolidGeometryByRaySegment &other) :
    ConstructiveSolidGeometry(other.getGeometryType()),
    topLevel(other.isTopLevel())
{
    for (long int i = 0; i < other.getOperands().size(); i++) {
        addOperand(static_cast<CsgOperand *>(other.getOperands()[i]->copy()));
    }
}

void *
ConstructiveSolidGeometryByRaySegment::copy()
{
    return new ConstructiveSolidGeometryByRaySegment(*this);
}

// One containment sample per child suffices: later crossings just toggle it
// ([ROTH1982].3.3 - a ray alternates in/out at each surface crossing).
Vector3Dd
ConstructiveSolidGeometryByRaySegment::classificationPoint(
    const RayWithTracingState &ray, java::PriorityQueue<IntersectionCandidate> &crossings)
{
    if (crossings.size() > 0) {
        IntersectionCandidate firstCandidate = crossings.peek();
        return ray.getOrigin().add(
            ray.getDirection().multiply(0.5 * firstCandidate.getIntersection().t));
    }
    // See [ROTH1982] Bug B (math/trough.pov): a continuation ray starting
    // exactly on this child's surface needs a positive offset, not a negative
    // tolerance, to land reliably on the correct side.
    return ray.getOrigin().add(ray.getDirection().multiply(2.0 * GeometryConfig::SMALL_TOLERANCE));
}

CsgSegmentClassification<IntersectionCandidate>
ConstructiveSolidGeometryByRaySegment::segmentsFromCrossings(
    java::PriorityQueue<IntersectionCandidate> &crossings, bool initialInside)
{
    CsgSegmentClassification<IntersectionCandidate> segments(initialInside, crossings.size());
    while (crossings.size() > 0) {
        IntersectionCandidate candidate = crossings.poll();
        segments.append(candidate.getIntersection().t, candidate);
    }
    return segments;
}

// Per-child RAYCAST classification ([ROTH1982].3.3, "In-Out Classification").
CsgSegmentClassification<IntersectionCandidate>
ConstructiveSolidGeometryByRaySegment::buildRaySegments(
    RayWithTracingState *ray, CsgOperand *child, Material *materialOverride)
{
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        ray->getIntersectionQueuePool()->pop(128);

    child->doIntersectionForAllRayCrossings(ray, localDepthQueue, materialOverride);
    const bool initialInside = child->doContainmentTest(
        classificationPoint(*ray, *localDepthQueue), 0.0) == RayCastingGeometry::INSIDE;
    CsgSegmentClassification<IntersectionCandidate> result = segmentsFromCrossings(*localDepthQueue, initialInside);

    ray->getIntersectionQueuePool()->push(localDepthQueue);
    return result;
}

// RAYCAST descends the tree and combines left/right classifications on the
// way back up ([ROTH1982].3.3), folded here over an n-ary children list.
int
ConstructiveSolidGeometryByRaySegment::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    const java::ArrayList<CsgOperand*> &children = getOperands();
    if (children.size() == 0) {
        return false;
    }

    if (isTopLevel() && isUnionOfBarePlanes(getGeometryType(), children)) {
        bool anyFound = false;
        for (long int i = 0; i < children.size(); i++) {
            if (children[i]->doIntersectionForAllRayCrossings(
                    ray, depthQueue, materialOverride)) {
                anyFound = true;
            }
        }
        return anyFound;
    }

    CsgSegmentClassification<IntersectionCandidate> result = buildRaySegments(
        ray, children[0], materialOverride);
    for (long int i = 1; i < children.size(); i++) {
        const CsgSegmentClassification<IntersectionCandidate> childSegments =
            buildRaySegments(ray, children[i], materialOverride);
        result = CsgSegmentClassification<IntersectionCandidate>::merge(result, childSegments, getGeometryType());
    }

    bool intersectionFound = false;
    const java::ArrayList<CsgSegmentEvent<IntersectionCandidate> > &resultEvents = result.getEvents();
    for (long int i = 0; i < resultEvents.size(); i++) {
        depthQueue->offer(resultEvents[i].getPayload());
        intersectionFound = true;
    }
    return intersectionFound;
}

int
ConstructiveSolidGeometryByRaySegment::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    const java::ArrayList<CsgOperand*> &children = getOperands();
    if (children.size() == 0) {
        return OUTSIDE;
    }

    // Table 3 of [ROTH1982].3.3 folded over the ordered operands, stopping as
    // soon as the remaining operands cannot change the result.
    const CsgOperation operation = getGeometryType();
    bool isInside = children[0]->doContainmentTest(point, distanceTolerance) != OUTSIDE;
    for (long int i = 1; i < children.size(); i++) {
        if (CsgMembershipRule::isDecided(operation, isInside)) {
            break;
        }
        isInside = CsgMembershipRule::combine(operation, isInside,
            children[i]->doContainmentTest(point, distanceTolerance) != OUTSIDE);
    }
    return isInside ? INSIDE : OUTSIDE;
}

// De Morgan complement of Table 3's three operators ([ROTH1982].3.3).
void
ConstructiveSolidGeometryByRaySegment::invertGeometry()
{
    const java::ArrayList<CsgOperand*> &children = getOperands();

    if (getGeometryType() == CsgOperation::INTERSECTION) {
        setGeometryType(CsgOperation::UNION);
        for (long int i = children.size() - 1; i >= 0; i--) {
            children[i]->invert();
        }
    } else if (getGeometryType() == CsgOperation::UNION) {
        setGeometryType(CsgOperation::INTERSECTION);
        for (long int i = children.size() - 1; i >= 0; i--) {
            children[i]->invert();
        }
    } else {
        setGeometryType(CsgOperation::UNION);
        if (children.size() > 0) {
            children[0]->invert();
        }
    }
}
