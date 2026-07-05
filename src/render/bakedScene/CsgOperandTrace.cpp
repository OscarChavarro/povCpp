
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "render/bakedScene/CsgMorganUnionTrace.h"
#include "render/bakedScene/RaySegmentCsgCombiner.h"
#include "render/bakedScene/SingleCorePlaneCsgTrace.h"

bool
CsgOperandTrace::tracePlanOperandAllCrossings(
    const BakedScene::CsgOperandRecord &operand,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (operand.compiledTransformedNestedCorePlane &&
        operand.nestedCsgProgramIndex >= 0 &&
        operand.nestedCsgProgramIndex < bakedCsgs.size() &&
        !ray->isPrimaryRayEnabled()) {
        Material *effectiveMaterial =
            operand.material != nullptr ? operand.material : materialOverride;
        return SingleCorePlaneCsgTrace::traceTransformedNestedSingleCorePlaneOperandAllCrossings(
            operand,
            bakedCsgs[operand.nestedCsgProgramIndex],
            bakedCsgs,
            ray,
            depthQueue,
            scratch.getCache(),
            effectiveMaterial);
    }

    return traceOperandAllCrossings(
        operand,
        bakedCsgs,
        scratch,
        ray,
        depthQueue,
        materialOverride);
}

bool
CsgOperandTrace::traceAllCrossingsWithScratch(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg.algorithm == BakedScene::CsgAlgorithm::RaySegments) {
        return RaySegmentCsgCombiner::traceRaySegmentCsg(
            bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
    }
    return CsgMorganUnionTrace::traceMorganCsg(
        bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
}

bool
CsgOperandTrace::traceAllCrossings(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache,
    Material *materialOverride)
{
    CsgScratchContext scratch(ray, &cache);
    const bool found = traceAllCrossingsWithScratch(
        bakedCsg,
        bakedCsgs,
        scratch,
        ray,
        depthQueue,
        materialOverride);
    scratch.releaseAll();
    return found;
}

// Explicit instantiation: guarantees a strong, always-emitted definition of
// PriorityQueuePool<IntersectionCandidate>::pop/push. Without this, marking
// those methods `inline` (for header-visible inlining at their many call
// sites) lets the optimizer fully inline every use it sees and elide the
// out-of-line copy in every translation unit, leaving TUs that only forward-
// declare the pool (e.g. BakedCompositeTracing.cpp) with an unresolved symbol.
template java::PriorityQueue<IntersectionCandidate> *
    PriorityQueuePool<IntersectionCandidate>::pop(int);
template void
    PriorityQueuePool<IntersectionCandidate>::push(java::PriorityQueue<IntersectionCandidate> *);
