#include "environment/intersection/ray/IntersectionTolerances.h"
#include "environment/scene/constructiveSolidGeometry/CsgRaySegmentEvaluator.h"
#include "vsdk/toolkit/environment/scene/csg/CsgMembershipRule.h"
#include "vsdk/toolkit/environment/scene/csg/CsgSegmentClassification.txx"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

bool
CsgRaySegmentEvaluator::isUnionOfBarePlanes(CsgOperation operation, const java::ArrayList<CsgOperand*> &operands)
{
    if (operation == CsgOperation::DIFFERENCE || operation == CsgOperation::INTERSECTION) {
        return false;
    }
    for (long int i = 0; i < operands.size(); i++) {
        if (operands[i]->getSurfaceFamily() != SurfaceDetailFamily::Plane) {
            return false;
        }
    }
    return true;
}

// One containment sample per operand suffices: later crossings just toggle it
// ([ROTH1982].3.3 - a ray alternates in/out at each surface crossing).
Vector3Dd
CsgRaySegmentEvaluator::classificationPoint(
    const RayWithTracingState &ray, java::PriorityQueue<IntersectionCandidate> &crossings)
{
    if (crossings.size() > 0) {
        IntersectionCandidate firstCandidate = crossings.peek();
        return ray.getOrigin().add(
            ray.getDirection().multiply(0.5 * firstCandidate.getIntersection().t));
    }
    // A continuation ray starting exactly on this operand's surface
    // (math/trough.pov) needs a positive offset, not a negative tolerance, to
    // land reliably on the correct side.
    return ray.getOrigin().add(ray.getDirection().multiply(2.0 * IntersectionTolerances::SMALL_TOLERANCE));
}

CsgSegmentClassification<IntersectionCandidate>
CsgRaySegmentEvaluator::segmentsFromCrossings(
    java::PriorityQueue<IntersectionCandidate> &crossings, bool initialInside)
{
    CsgSegmentClassification<IntersectionCandidate> segments(initialInside, crossings.size());
    while (crossings.size() > 0) {
        IntersectionCandidate candidate = crossings.poll();
        segments.append(candidate.getIntersection().t, candidate);
    }
    return segments;
}

// Per-operand RAYCAST classification ([ROTH1982].3.3, "In-Out Classification").
CsgSegmentClassification<IntersectionCandidate>
CsgRaySegmentEvaluator::buildRaySegments(
    RayWithTracingState *ray, CsgOperand *operand, Material *materialOverride)
{
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        ray->getIntersectionQueuePool()->pop(128);

    operand->doIntersectionForAllRayCrossings(ray, localDepthQueue, materialOverride);
    const bool initialInside = operand->doContainmentTest(
        classificationPoint(*ray, *localDepthQueue), 0.0) == RayCastingContainment::INSIDE;
    CsgSegmentClassification<IntersectionCandidate> result = segmentsFromCrossings(*localDepthQueue, initialInside);

    ray->getIntersectionQueuePool()->push(localDepthQueue);
    return result;
}

// RAYCAST descends the tree and combines left/right classifications on the
// way back up ([ROTH1982].3.3), folded here over an n-ary operand list.
int
CsgRaySegmentEvaluator::allRayCrossings(
    CsgOperation operation,
    const java::ArrayList<CsgOperand*> &operands,
    bool topLevel,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (operands.size() == 0) {
        return false;
    }

    if (topLevel && isUnionOfBarePlanes(operation, operands)) {
        bool anyFound = false;
        for (long int i = 0; i < operands.size(); i++) {
            if (operands[i]->doIntersectionForAllRayCrossings(ray, depthQueue, materialOverride)) {
                anyFound = true;
            }
        }
        return anyFound;
    }

    CsgSegmentClassification<IntersectionCandidate> result = buildRaySegments(
        ray, operands[0], materialOverride);
    for (long int i = 1; i < operands.size(); i++) {
        const CsgSegmentClassification<IntersectionCandidate> operandSegments =
            buildRaySegments(ray, operands[i], materialOverride);
        result = CsgSegmentClassification<IntersectionCandidate>::merge(result, operandSegments, operation);
    }

    bool intersectionFound = false;
    const java::ArrayList<CsgSegmentEvent<IntersectionCandidate> > &resultEvents = result.getEvents();
    for (long int i = 0; i < resultEvents.size(); i++) {
        depthQueue->offer(resultEvents[i].getPayload());
        intersectionFound = true;
    }
    return intersectionFound;
}

bool
CsgRaySegmentEvaluator::contains(
    CsgOperation operation,
    const java::ArrayList<CsgOperand*> &operands,
    const Vector3Dd &point,
    double distanceTolerance)
{
    if (operands.size() == 0) {
        return false;
    }

    // Table 3 of [ROTH1982].3.3 folded over the ordered operands, stopping as
    // soon as the remaining operands cannot change the result.
    bool isInside = operands[0]->doContainmentTest(point, distanceTolerance) != RayCastingContainment::OUTSIDE;
    for (long int i = 1; i < operands.size(); i++) {
        if (CsgMembershipRule::isDecided(operation, isInside)) {
            break;
        }
        isInside = CsgMembershipRule::combine(operation, isInside,
            operands[i]->doContainmentTest(point, distanceTolerance) != RayCastingContainment::OUTSIDE);
    }
    return isInside;
}

CsgOperation
CsgRaySegmentEvaluator::invert(CsgOperation operation, const java::ArrayList<CsgOperand*> &operands)
{
    if (operation == CsgOperation::INTERSECTION || operation == CsgOperation::UNION) {
        for (long int i = operands.size() - 1; i >= 0; i--) {
            operands[i]->invert();
        }
        return operation == CsgOperation::INTERSECTION ? CsgOperation::UNION : CsgOperation::INTERSECTION;
    }
    if (operands.size() > 0) {
        operands[0]->invert();
    }
    return CsgOperation::UNION;
}
