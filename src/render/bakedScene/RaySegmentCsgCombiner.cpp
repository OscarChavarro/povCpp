
#include "render/bakedScene/RaySegmentCsgCombiner.h"

#include "render/bakedScene/CsgContainmentTest.h"
#include "render/bakedScene/CsgOperandTrace.h"
#include "render/bakedScene/SingleCorePlaneCsgTrace.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.h"
#include "vsdk/toolkit/environment/scene/csg/CsgSegmentClassification.txx"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

CsgSegmentClassification<IntersectionCandidate>
RaySegmentCsgCombiner::buildRaySegments(
    const CsgOperandRecord *operand,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
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

    const bool initialInside =
        CsgContainmentTest::containmentTestOperand(
            operand,
            bakedCsgs,
            ConstructiveSolidGeometryByRaySegment::classificationPoint(*ray, *localDepthQueue),
            0.0) == RayCastingGeometry::INSIDE;
    CsgSegmentClassification<IntersectionCandidate> result =
        ConstructiveSolidGeometryByRaySegment::segmentsFromCrossings(*localDepthQueue, initialInside);

    scratch.returnQueue(localDepthQueue);
    return result;
}

int
RaySegmentCsgCombiner::traceRaySegmentCsg(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg->getOperands().size() == 0) {
        return false;
    }

    if (bakedCsg->getPlanKind() ==
            BakedScene::CsgPlanKind::TopLevelPlaneUnion ||
        bakedCsg->getPlanKind() ==
            BakedScene::CsgPlanKind::DisjointBoundedUnion) {
        bool anyFound = false;
        for (long int i = 0; i < bakedCsg->getOperands().size(); i++) {
            if (CsgOperandTrace::tracePlanOperandAllCrossings(
                    &bakedCsg->getOperands()[i],
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

    if (bakedCsg->getPlanKind() ==
            BakedScene::CsgPlanKind::SingleCorePlaneIntersection &&
        bakedCsg->getSpecializationValid()) {
        return SingleCorePlaneCsgTrace::traceSingleCorePlaneIntersection(
            bakedCsg,
            bakedCsgs,
            scratch,
            ray,
            depthQueue,
            materialOverride,
            bakedCsg->getSpecializationCoreOperandIndex());
    }

    CsgSegmentClassification<IntersectionCandidate> result = buildRaySegments(
        &bakedCsg->getOperands()[0], bakedCsgs, scratch, ray, materialOverride);
    for (long int i = 1; i < bakedCsg->getOperands().size(); i++) {
        const CsgSegmentClassification<IntersectionCandidate> childSegments =
            buildRaySegments(
                &bakedCsg->getOperands()[i], bakedCsgs, scratch, ray, materialOverride);
        result = CsgSegmentClassification<IntersectionCandidate>::merge(
            result, childSegments, bakedCsg->getGeometryType());
    }

    bool intersectionFound = false;
    const java::ArrayList<CsgSegmentEvent<IntersectionCandidate> > &resultEvents = result.getEvents();
    for (long int i = 0; i < resultEvents.size(); i++) {
        depthQueue->offer(resultEvents[i].getPayload());
        intersectionFound = true;
    }
    return intersectionFound;
}
