#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "render/bakedScene/RaySegmentCsgCombiner.h"

#include "common/Config.h"
#include "environment/geometry/Geometry.h"
#include "render/bakedScene/CsgContainmentTest.h"
#include "render/bakedScene/CsgOperandTrace.h"
#include "render/bakedScene/SingleCorePlaneCsgTrace.h"

RaySegments
RaySegmentCsgCombiner::buildRaySegments(
    const BakedScene::CsgOperandRecord &operand,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    Material *materialOverride)
{
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();

    CsgOperandTrace::tracePlanOperandAllCrossings(
        operand,
        bakedCsgs,
        scratch,
        ray,
        localDepthQueue,
        materialOverride);

    bool initialInside;
    if (localDepthQueue->size() > 0) {
        IntersectionCandidate firstCandidate = localDepthQueue->peek();
        Vector3Dd samplePoint = ray->getOrigin().add(
            ray->getDirection().multiply(0.5 * firstCandidate.getIntersection().t));
        initialInside =
            CsgContainmentTest::containmentTestOperand(
                operand, bakedCsgs, samplePoint, 0.0) == Geometry::INSIDE;
    } else {
        Vector3Dd samplePoint =
            ray->getOrigin().add(ray->getDirection().multiply(2.0 * Config::SMALL_TOLERANCE));
        initialInside =
            CsgContainmentTest::containmentTestOperand(
                operand, bakedCsgs, samplePoint, 0.0) == Geometry::INSIDE;
    }

    java::ArrayList<RaySegmentCrossing> crossings{localDepthQueue->size()};
    bool currentlyInside = initialInside;
    while (localDepthQueue->size() > 0) {
        IntersectionCandidate candidate = localDepthQueue->poll();
        currentlyInside = !currentlyInside;
        crossings.add(RaySegmentCrossing(candidate.getIntersection().t, currentlyInside, candidate));
    }

    scratch.returnQueue(localDepthQueue);
    return RaySegments(crossings, initialInside);
}

bool
RaySegmentCsgCombiner::combineUnion(bool insideLeft, bool insideRight)
{
    return insideLeft || insideRight;
}

bool
RaySegmentCsgCombiner::combineIntersection(bool insideLeft, bool insideRight)
{
    return insideLeft && insideRight;
}

bool
RaySegmentCsgCombiner::combineDifference(bool insideLeft, bool insideRight)
{
    return insideLeft && !insideRight;
}

RaySegments
RaySegmentCsgCombiner::mergeByMembership(
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

int
RaySegmentCsgCombiner::traceRaySegmentCsg(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg.operands.size() == 0) {
        return false;
    }

    if (bakedCsg.planKind ==
            BakedScene::CsgPlanKind::TopLevelPlaneUnion ||
        bakedCsg.planKind ==
            BakedScene::CsgPlanKind::DisjointBoundedUnion) {
        bool anyFound = false;
        for (long int i = 0; i < bakedCsg.operands.size(); i++) {
            if (CsgOperandTrace::tracePlanOperandAllCrossings(
                    bakedCsg.operands[i],
                    bakedCsgs,
                    scratch,
                    ray,
                    depthQueue,
                    materialOverride)) {
                anyFound = true;
            }
        }
        return anyFound;
    }

    if (bakedCsg.planKind ==
            BakedScene::CsgPlanKind::SingleCorePlaneIntersection &&
        bakedCsg.specializationValid) {
        return SingleCorePlaneCsgTrace::traceSingleCorePlaneIntersection(
            bakedCsg,
            bakedCsgs,
            scratch,
            ray,
            depthQueue,
            materialOverride,
            bakedCsg.specializationCoreOperandIndex);
    }

    RaySegments result = buildRaySegments(
        bakedCsg.operands[0], bakedCsgs, scratch, ray, materialOverride);
    for (long int i = 1; i < bakedCsg.operands.size(); i++) {
        const RaySegments childSegments =
            buildRaySegments(
                bakedCsg.operands[i], bakedCsgs, scratch, ray, materialOverride);
        switch (bakedCsg.geometryType) {
        case BooleanSetOperations::DIFFERENCE:
            result = mergeByMembership(result, childSegments, combineDifference);
            break;
        case BooleanSetOperations::INTERSECTION:
            result = mergeByMembership(result, childSegments, combineIntersection);
            break;
        default:
            result = mergeByMembership(result, childSegments, combineUnion);
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
