
#include "render/bakedScene/CsgMorganUnionTrace.h"

#include "render/bakedScene/SingleCorePlaneCsgTrace.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

int
CsgMorganUnionTrace::traceMorganIntersection(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
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

    return traceMorganIntersectionGeneric(
        bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
}

int
CsgMorganUnionTrace::traceMorganCsg(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg->getGeometryType() == BooleanSetOperations::INTERSECTION) {
        return traceMorganIntersection(
            bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
    }

    if (bakedCsg->getGeometryType() == BooleanSetOperations::UNION &&
        bakedCsg->getPlanKind() == BakedScene::CsgPlanKind::GenericMorgan) {
        if (bakedCsg->getDirectPrimitiveCullBins() != nullptr ||
            bakedCsg->getTransformedPrimitiveCullBins() != nullptr) {
            return traceGenericMorganUnionWithCullBins(
                bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
        }
        return traceGenericMorganUnion(
            bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
    }

    bool intersectionFound = false;
    for (long int i = bakedCsg->getOperands().size() - 1; i >= 0; i--) {
        if (CsgOperandTrace::tracePlanOperandAllCrossings(
                &bakedCsg->getOperands()[i],
                bakedCsgs,
                scratch,
                ray,
                depthQueue,
                materialOverride)) {
            intersectionFound = true;
        }
    }
    return intersectionFound;
}
