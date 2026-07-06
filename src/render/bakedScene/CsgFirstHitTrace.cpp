
#include "render/bakedScene/CsgFirstHitTrace.h"

#include "render/bakedScene/CsgContainmentTest.h"
#include "render/bakedScene/CsgOperandTrace.h"
#include "render/bakedScene/SingleCorePlaneCsgTrace.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

bool
CsgFirstHitTrace::traceFirstHitByIntersectionMembership(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();
    bool found = false;
    double bestT = GeometryConfig::MAX_DISTANCE;

    for (long int i = bakedCsg->getOperands().size() - 1; i >= 0; i--) {
        localDepthQueue->clear();
        CsgOperandTrace::tracePlanOperandAllCrossings(
            bakedCsg->getOperands()[i],
            bakedCsgs,
            scratch,
            ray,
            localDepthQueue,
            materialOverride);

        for (IntersectionCandidate &candidate : *localDepthQueue) {
            const double t = candidate.getIntersection().t;
            if (t <= GeometryConfig::SMALL_TOLERANCE || t >= bestT) {
                continue;
            }
            if (!CsgContainmentTest::candidateInsideAllOtherOperands(
                    bakedCsg,
                    bakedCsgs,
                    candidate.getIntersection().point,
                    i)) {
                continue;
            }
            out = candidate;
            bestT = t;
            found = true;
        }
    }

    scratch.returnQueue(localDepthQueue);
    return found;
}

bool
CsgFirstHitTrace::traceFirstHitWithScratch(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    if (bakedCsg->getOperands().size() == 0) {
        return false;
    }

    if (bakedCsg->getGeometryType() == BooleanSetOperations::INTERSECTION) {
        if (bakedCsg->getPlanKind() ==
                BakedScene::CsgPlanKind::SingleCorePlaneIntersection &&
            bakedCsg->getSpecializationValid()) {
            return SingleCorePlaneCsgTrace::traceFirstHitCompiledSingleCorePlane(
                bakedCsg,
                bakedCsgs,
                scratch,
                ray,
                out,
                materialOverride);
        }
        return traceFirstHitByIntersectionMembership(
            bakedCsg, bakedCsgs, scratch, ray, out, materialOverride);
    }

    java::PriorityQueue<IntersectionCandidate> *depthQueue =
        scratch.borrowQueue();
    const bool found = CsgOperandTrace::traceAllCrossingsWithScratch(
        bakedCsg,
        bakedCsgs,
        scratch,
        ray,
        depthQueue,
        materialOverride) &&
        depthQueue->size() > 0;
    if (found) {
        out = depthQueue->peek();
    }
    scratch.returnQueue(depthQueue);
    return found;
}

bool
CsgFirstHitTrace::traceFirstHit(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    RaySharedCache &cache,
    Material *materialOverride)
{
    CsgScratchContext scratch(ray, &cache);
    const bool found = traceFirstHitWithScratch(
        bakedCsg,
        bakedCsgs,
        scratch,
        ray,
        out,
        materialOverride);
    scratch.releaseAll();
    return found;
}
