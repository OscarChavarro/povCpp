
#include "render/bakedScene/CsgMorganUnionTrace.h"

#include "render/bakedScene/SingleCorePlaneCsgTrace.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

int
CsgMorganUnionTrace::traceMorganIntersection(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
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

    return traceMorganIntersectionGeneric(
        bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
}

int
CsgMorganUnionTrace::traceMorganCsg(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg.geometryType == BooleanSetOperations::INTERSECTION) {
        return traceMorganIntersection(
            bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
    }

    if (bakedCsg.geometryType == BooleanSetOperations::UNION &&
        bakedCsg.planKind == BakedScene::CsgPlanKind::GenericMorgan) {
        if (bakedCsg.directPrimitiveCullBins != nullptr ||
            bakedCsg.transformedPrimitiveCullBins != nullptr) {
            return traceGenericMorganUnionWithCullBins(
                bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
        }
        return traceGenericMorganUnion(
            bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
    }

    bool intersectionFound = false;
    for (long int i = bakedCsg.operands.size() - 1; i >= 0; i--) {
        if (CsgOperandTrace::tracePlanOperandAllCrossings(
                bakedCsg.operands[i],
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
