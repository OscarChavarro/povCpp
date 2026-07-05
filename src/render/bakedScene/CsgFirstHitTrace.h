#ifndef __CSG_FIRST_HIT_TRACE__
#define __CSG_FIRST_HIT_TRACE__

#include "render/bakedScene/BakedScene.h"
#include "render/bakedScene/CsgScratchContext.h"

// Public "first hit" API for baked CSG programs: finds the single closest
// valid crossing along the ray, dispatching to the compiled single-core-
// plane fast path (SingleCorePlaneCsgTrace) when available, and otherwise
// falling back to either intersection-by-membership scanning or the
// generic all-crossings trace (CsgOperandTrace) plus taking its nearest
// candidate.
class CsgFirstHitTrace {
public:
    static bool traceFirstHit(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        RayWithTracingState *ray,
        IntersectionCandidate &out,
        RaySharedCache &cache,
        Material *materialOverride = nullptr);

    static bool traceFirstHitWithScratch(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        IntersectionCandidate &out,
        Material *materialOverride);

private:
    static bool traceFirstHitByIntersectionMembership(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        IntersectionCandidate &out,
        Material *materialOverride);
};

#endif
