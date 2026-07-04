#ifndef __CSG_MORGAN_UNION_TRACE__
#define __CSG_MORGAN_UNION_TRACE__

#include "common/Config.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/Material.h"
#include "java/util/ArrayList.h"
#include "java/util/PriorityQueue.h"
#include "render/bakedScene/AabbCullingSupport.h"
#include "render/bakedScene/BakedPlaneIntersector.h"
#include "render/bakedScene/BakedQuadricIntersector.h"
#include "render/bakedScene/BakedScene.h"
#include "render/bakedScene/CsgContainmentTest.h"
#include "render/bakedScene/CsgOperandTrace.h"
#include "render/bakedScene/CsgScratchContext.h"

// Fast path for wide unions and De Morgan-rewritten intersections: instead
// of the generic per-operand dispatch (CsgOperandTrace), this class walks
// baked per-kind operand buckets (planes / direct primitives / nested CSGs
// / transformed primitives) directly, optionally skipping most operands via
// a bake-time AABB cull-bins index (Plan 13) for scenes with wide fan-out
// (spline/ntreal/piece3-class unions).
class CsgMorganUnionTrace {
public:
    static int traceMorganCsg(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static int traceMorganIntersection(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    // Plan 7 Phase 4: prescan and main loop both need each TransformedQuadric
    // operand's local-space intersection - without this cache the transform
    // (2 matrix-vector ops) and the quadric solve (~40 FLOPs) ran twice per
    // operand per ray. Cache scope is exactly this one function call (one CSG
    // program, one ray): a plain stack array indexed by the loop index `i`,
    // not the global RaySharedCache - no cross-call generation-stamp risk,
    // since the array cannot outlive the call that filled it. Capped at
    // MAX_CACHED_QUADRIC_OPERANDS; CSGs with more top-level operands than
    // that just skip the optimization and re-trace as before (always
    // correct, only forgoes the speedup).
    static constexpr long int MAX_CACHED_QUADRIC_OPERANDS = 64;

    static inline int traceMorganIntersectionGeneric(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        const long int operandCount = bakedCsg.operands.size();
        const bool canCache = operandCount <= MAX_CACHED_QUADRIC_OPERANDS;
        bool cachedValid[MAX_CACHED_QUADRIC_OPERANDS];
        bool cachedHit[MAX_CACHED_QUADRIC_OPERANDS];
        Vector3Dd cachedLocalOrigin[MAX_CACHED_QUADRIC_OPERANDS];
        Vector3Dd cachedLocalDirection[MAX_CACHED_QUADRIC_OPERANDS];
        double cachedDepth1[MAX_CACHED_QUADRIC_OPERANDS];
        double cachedDepth2[MAX_CACHED_QUADRIC_OPERANDS];
        if (canCache) {
            for (long int i = 0; i < operandCount; i++) {
                cachedValid[i] = false;
            }
        }

        // Pre-scan: if any positive-quadric (polyA>0) operand definitely misses the ray
        // (trueMiss), all points along the ray are outside that operand. No crossing from
        // any other operand can satisfy the containment check for it → the whole INTERSECTION
        // produces no valid candidates. Early exit saves 3 operand evals per X_Tube miss.
        for (long int i = 0; i < operandCount; i++) {
            const BakedScene::CsgOperandRecord &op = bakedCsg.operands[i];
            if (op.kind != BakedScene::CsgOperandKind::TransformedQuadric ||
                op.quadricGeometry == nullptr) {
                continue;
            }
            const Vector3Dd localOrigin =
                op.localToObject.transformPoint(ray->getOrigin());
            const Vector3Dd localDirection =
                op.localToObject.transformDirection(ray->getDirection());
            double polyA = 0, polyB = 0, polyC = 0;
            bool trueMiss = false;
            double d1, d2;
            const bool hit = BakedQuadricIntersector::intersectBakedQuadricWithCoeffs(
                *op.quadricGeometry, ray, localOrigin, localDirection,
                false, scratch.getCache(), op.quadricViewpointSlot,
                &d1, &d2, polyA, polyB, polyC, trueMiss);
            if (trueMiss && polyA > 0.0) {
                return false;
            }
            if (canCache) {
                cachedValid[i] = true;
                cachedHit[i] = hit;
                cachedLocalOrigin[i] = localOrigin;
                cachedLocalDirection[i] = localDirection;
                cachedDepth1[i] = d1;
                cachedDepth2[i] = d2;
            }
        }

        java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
            scratch.borrowQueue();
        bool anyIntersectionFound = false;

        for (long int i = bakedCsg.operands.size() - 1; i >= 0; i--) {
            const BakedScene::CsgOperandRecord &localShape = bakedCsg.operands[i];
            if (canCache && cachedValid[i]) {
                // Reuse the prescan's transform + quadric solve verbatim -
                // this mirrors CsgOperandTrace::traceOperandAllCrossings's
                // TransformedQuadric branch exactly (same effectiveMaterial
                // rule, same offerTransformedPrimitiveCandidate calls), just
                // fed from the cache instead of recomputing.
                if (cachedHit[i]) {
                    Material *effectiveMaterial =
                        localShape.material != nullptr ? localShape.material : materialOverride;
                    CsgOperandTrace::offerTransformedPrimitiveCandidate(
                        localShape, ray, effectiveMaterial,
                        cachedLocalOrigin[i], cachedLocalDirection[i],
                        cachedDepth1[i], localDepthQueue);
                    if (cachedDepth2[i] != cachedDepth1[i]) {
                        CsgOperandTrace::offerTransformedPrimitiveCandidate(
                            localShape, ray, effectiveMaterial,
                            cachedLocalOrigin[i], cachedLocalDirection[i],
                            cachedDepth2[i], localDepthQueue);
                    }
                }
            } else {
                CsgOperandTrace::tracePlanOperandAllCrossings(
                    localShape, bakedCsgs, scratch, ray, localDepthQueue, materialOverride);
            }

            for (IntersectionCandidate &candidate : *localDepthQueue) {
                if (CsgContainmentTest::candidateInsideAllOtherOperands(
                        bakedCsg,
                        bakedCsgs,
                        candidate.getIntersection().point,
                        i)) {
                    depthQueue->offer(candidate);
                    anyIntersectionFound = true;
                }
            }
            localDepthQueue->clear();
        }

        scratch.returnQueue(localDepthQueue);
        return anyIntersectionFound;
    }

    // Plan 13 Phase 1: dispatch body factored out of
    // traceGenericMorganUnion's direct-primitives bucket so the built/
    // not-built loops below share one copy of it instead of two - an
    // earlier version duplicated this body in both branches, which grew
    // traceGenericMorganUnion (the hottest, most frequently called frame in
    // the CSG trace - tens of millions of calls in drums/iortest) enough to
    // regress those scenes ~6-10% purely from the larger static code size,
    // even though neither scene ever takes the "built" branch at all (see
    // doc/performanceReviewPlan13.md Phase 1).
    static inline void dispatchDirectPrimitiveOperand(
        const BakedScene::CsgOperandRecord &operand,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        bool &anyFound)
    {
        if (operand.geometry == nullptr) {
            return;
        }
        Material *effectiveMaterial =
            operand.material != nullptr ? operand.material : materialOverride;
        if (operand.kind == BakedScene::CsgOperandKind::DirectAnnotatedPrimitive) {
            GeometryIntersectionEmissionContext context;
            context.materialOverride = effectiveMaterial;
            context.detailOwner = operand.operand;
            context.materialUsesObjectLocalPoint = true;
            if (operand.geometry->doIntersectionForAllRayCrossingsAnnotated(
                    ray, depthQueue, context)) {
                anyFound = true;
            }
        } else {
            const int initialSize = depthQueue->size();
            if (operand.geometry->doIntersectionForAllRayCrossings(
                    ray, depthQueue, effectiveMaterial) &&
                depthQueue->size() > initialSize) {
                if (CsgOperandTrace::annotateDirectCandidates(depthQueue, operand)) {
                    anyFound = true;
                }
            }
        }
    }

    // Same rationale as dispatchDirectPrimitiveOperand above, for the
    // transformed-primitives bucket's TransformedQuadric fast path +
    // generic CsgOperandTrace::traceOperandAllCrossings fallback.
    static inline void dispatchTransformedPrimitiveOperand(
        const BakedScene::CsgOperandRecord &operand,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        bool &anyFound)
    {
        if (operand.kind == BakedScene::CsgOperandKind::TransformedQuadric) {
            if (operand.geometry == nullptr || operand.quadricGeometry == nullptr) {
                return;
            }
            Material *effectiveMaterial =
                operand.material != nullptr ? operand.material : materialOverride;
            const Vector3Dd localOrigin =
                operand.localToObject.transformPoint(ray->getOrigin());
            const Vector3Dd localDirection =
                operand.localToObject.transformDirection(ray->getDirection());
            double depth1;
            double depth2;
            if (!BakedQuadricIntersector::intersectBakedQuadric(
                    *operand.quadricGeometry, ray, localOrigin, localDirection,
                    false, scratch.getCache(), operand.quadricViewpointSlot,
                    &depth1, &depth2)) {
                return;
            }
            CsgOperandTrace::offerTransformedPrimitiveCandidate(
                operand, ray, effectiveMaterial,
                localOrigin, localDirection, depth1, depthQueue);
            anyFound = true;
            if (depth2 != depth1) {
                CsgOperandTrace::offerTransformedPrimitiveCandidate(
                    operand, ray, effectiveMaterial,
                    localOrigin, localDirection, depth2, depthQueue);
            }
            return;
        }
        if (CsgOperandTrace::traceOperandAllCrossings(
                operand, bakedCsgs, scratch, ray, depthQueue, materialOverride)) {
            anyFound = true;
        }
    }

    // Plan 13 Phase 1: kept byte-for-byte identical to the pre-Plan-13 code
    // (no cull-bins references at all). traceMorganCsg picks this one
    // whenever neither operand bucket has a built cull index, so scenes
    // like drums/iortest - whose union programs never reach the fan-out
    // threshold (see doc/performanceReviewPlan13.md Phase 0 census) -
    // execute the exact same instructions as before Plan 13, with zero risk
    // of the struct-growth/code-size regression that motivated splitting
    // this out (see traceGenericMorganUnionWithCullBins's comment).
    static inline bool traceGenericMorganUnion(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        bool anyFound = false;

        // Direct planes: inline emission, no dispatcher overhead.
        for (long int p = bakedCsg.planeOperandIndices.size() - 1;
             p >= 0; p--) {
            const BakedScene::CsgOperandRecord &operand =
                bakedCsg.operands[bakedCsg.planeOperandIndices[p]];
            IntersectionCandidate candidate;
            if (BakedPlaneIntersector::tracePlaneOperandCandidate(
                    operand, ray, scratch.getCache(), materialOverride, candidate)) {
                depthQueue->offer(candidate);
                anyFound = true;
            }
        }

        // Direct primitives: bounds check then direct geometry call.
        for (long int d = bakedCsg.directPrimitiveOperandIndices.size() - 1;
             d >= 0; d--) {
            const BakedScene::CsgOperandRecord &operand =
                bakedCsg.operands[bakedCsg.directPrimitiveOperandIndices[d]];
            if (operand.geometry == nullptr) {
                continue;
            }
            if (operand.bounded && operand.cullSafe &&
                !AabbCullingSupport::rayIntersectsAabbForward(*ray, operand.bakedBounds)) {
                continue;
            }
            Material *effectiveMaterial =
                operand.material != nullptr ? operand.material : materialOverride;
            if (operand.kind ==
                BakedScene::CsgOperandKind::DirectAnnotatedPrimitive) {
                GeometryIntersectionEmissionContext context;
                context.materialOverride = effectiveMaterial;
                context.detailOwner = operand.operand;
                context.materialUsesObjectLocalPoint = true;
                if (operand.geometry->doIntersectionForAllRayCrossingsAnnotated(
                        ray, depthQueue, context)) {
                    anyFound = true;
                }
            } else {
                const int initialSize = depthQueue->size();
                if (operand.geometry->doIntersectionForAllRayCrossings(
                        ray, depthQueue, effectiveMaterial) &&
                    depthQueue->size() > initialSize) {
                    if (CsgOperandTrace::annotateDirectCandidates(depthQueue, operand)) {
                        anyFound = true;
                    }
                }
            }
        }

        // Nested CSGs: use compiled dispatch (handles compiled vs. generic per ray kind).
        for (long int n = bakedCsg.nestedOperandIndices.size() - 1;
             n >= 0; n--) {
            if (CsgOperandTrace::tracePlanOperandAllCrossings(
                    bakedCsg.operands[bakedCsg.nestedOperandIndices[n]],
                    bakedCsgs,
                    scratch,
                    ray,
                    depthQueue,
                    materialOverride)) {
                anyFound = true;
            }
        }

        // Transformed primitives and quadrics: TransformedQuadric (the
        // dominant residual-transformed kind) gets a direct fused branch
        // instead of paying traceOperandAllCrossings's function-call
        // boundary and generic kind dispatch (Plan 8 Phase 1); everything
        // else keeps the generic path. Iteration order is unchanged (same
        // array, same direction) so candidate/tie-break ordering across
        // operand kinds is preserved exactly.
        for (long int t = bakedCsg.transformedPrimitiveOperandIndices.size() - 1;
             t >= 0; t--) {
            const BakedScene::CsgOperandRecord &operand =
                bakedCsg.operands[bakedCsg.transformedPrimitiveOperandIndices[t]];
            if (operand.kind == BakedScene::CsgOperandKind::TransformedQuadric) {
                if (operand.geometry == nullptr || operand.quadricGeometry == nullptr) {
                    continue;
                }
                if (operand.bounded && operand.cullSafe &&
                    !AabbCullingSupport::rayIntersectsAabbForward(*ray, operand.bakedBounds)) {
                    continue;
                }
                Material *effectiveMaterial =
                    operand.material != nullptr ? operand.material : materialOverride;
                const Vector3Dd localOrigin =
                    operand.localToObject.transformPoint(ray->getOrigin());
                const Vector3Dd localDirection =
                    operand.localToObject.transformDirection(ray->getDirection());
                double depth1;
                double depth2;
                if (!BakedQuadricIntersector::intersectBakedQuadric(
                        *operand.quadricGeometry, ray, localOrigin, localDirection,
                        false, scratch.getCache(), operand.quadricViewpointSlot,
                        &depth1, &depth2)) {
                    continue;
                }
                CsgOperandTrace::offerTransformedPrimitiveCandidate(
                    operand, ray, effectiveMaterial,
                    localOrigin, localDirection, depth1, depthQueue);
                anyFound = true;
                if (depth2 != depth1) {
                    CsgOperandTrace::offerTransformedPrimitiveCandidate(
                        operand, ray, effectiveMaterial,
                        localOrigin, localDirection, depth2, depthQueue);
                }
                continue;
            }
            if (CsgOperandTrace::traceOperandAllCrossings(
                    operand,
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

    // Plan 13 Phase 1: only called when at least one bucket has a built
    // OperandCullBins (wide unions - spline/ntreal/piece3 class scenes;
    // see doc/performanceReviewPlan13.md Phase 0 census). Uses the bins to
    // skip most per-operand AABB tests instead of scanning every operand
    // linearly; dispatch itself is unchanged (dispatchDirectPrimitiveOperand/
    // dispatchTransformedPrimitiveOperand, shared with traceGenericMorganUnion's
    // own fallback loops for the OTHER bucket, if only one of the two is built).
    static inline bool traceGenericMorganUnionWithCullBins(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        bool anyFound = false;

        for (long int p = bakedCsg.planeOperandIndices.size() - 1;
             p >= 0; p--) {
            const BakedScene::CsgOperandRecord &operand =
                bakedCsg.operands[bakedCsg.planeOperandIndices[p]];
            IntersectionCandidate candidate;
            if (BakedPlaneIntersector::tracePlaneOperandCandidate(
                    operand, ray, scratch.getCache(), materialOverride, candidate)) {
                depthQueue->offer(candidate);
                anyFound = true;
            }
        }

        // Plan 13 Phase 1: thread_local, not a CsgScratchContext member -
        // CsgScratchContext is stack-constructed on every top-level
        // traceAllCrossings/traceFirstHit call (up to ~1M times for drums),
        // and giving it two 4096-int members measurably cost several
        // percent even fully unused (likely cold-stack-page first-touch
        // cost), on top of the even larger hit from declaring them as
        // per-call locals inside this function directly (tried first, see
        // doc/performanceReviewPlan13.md Phase 1). A thread_local pays for
        // the storage exactly once per render worker thread. Reuse across
        // nested-union recursion within one thread is safe for the same
        // reason a CsgScratchContext-owned buffer would have been: each use
        // is fully drained (gather -> sort -> dispatch) before any nested
        // call that could re-enter and overwrite it runs.
        thread_local int directPositionsStorage[AabbCullingSupport::OPERAND_CULL_SCRATCH_CAPACITY];
        int *directPositions = directPositionsStorage;
        int directCount = -1;
        if (bakedCsg.directPrimitiveCullBins != nullptr) {
            directCount = AabbCullingSupport::gatherCullSurvivors(
                *bakedCsg.directPrimitiveCullBins,
                bakedCsg.directPrimitiveOperandIndices,
                bakedCsg.operands,
                *ray, directPositions, AabbCullingSupport::OPERAND_CULL_SCRATCH_CAPACITY);
            if (directCount >= 0) {
                AabbCullingSupport::sortPositionsDescending(directPositions, directCount);
            }
        }
        if (directCount >= 0) {
            for (int idx = 0; idx < directCount; idx++) {
                const long int d = directPositions[idx];
                dispatchDirectPrimitiveOperand(
                    bakedCsg.operands[bakedCsg.directPrimitiveOperandIndices[d]],
                    ray, depthQueue, materialOverride, anyFound);
            }
        } else {
            for (long int d = bakedCsg.directPrimitiveOperandIndices.size() - 1;
                 d >= 0; d--) {
                const BakedScene::CsgOperandRecord &operand =
                    bakedCsg.operands[bakedCsg.directPrimitiveOperandIndices[d]];
                if (operand.bounded && operand.cullSafe &&
                    !AabbCullingSupport::rayIntersectsAabbForward(*ray, operand.bakedBounds)) {
                    continue;
                }
                dispatchDirectPrimitiveOperand(
                    operand, ray, depthQueue, materialOverride, anyFound);
            }
        }

        for (long int n = bakedCsg.nestedOperandIndices.size() - 1;
             n >= 0; n--) {
            if (CsgOperandTrace::tracePlanOperandAllCrossings(
                    bakedCsg.operands[bakedCsg.nestedOperandIndices[n]],
                    bakedCsgs,
                    scratch,
                    ray,
                    depthQueue,
                    materialOverride)) {
                anyFound = true;
            }
        }

        thread_local int transformedPositionsStorage[AabbCullingSupport::OPERAND_CULL_SCRATCH_CAPACITY];
        int *transformedPositions = transformedPositionsStorage;
        int transformedCount = -1;
        if (bakedCsg.transformedPrimitiveCullBins != nullptr) {
            transformedCount = AabbCullingSupport::gatherCullSurvivors(
                *bakedCsg.transformedPrimitiveCullBins,
                bakedCsg.transformedPrimitiveOperandIndices,
                bakedCsg.operands,
                *ray, transformedPositions, AabbCullingSupport::OPERAND_CULL_SCRATCH_CAPACITY);
            if (transformedCount >= 0) {
                AabbCullingSupport::sortPositionsDescending(transformedPositions, transformedCount);
            }
        }
        if (transformedCount >= 0) {
            for (int idx = 0; idx < transformedCount; idx++) {
                const long int t = transformedPositions[idx];
                dispatchTransformedPrimitiveOperand(
                    bakedCsg.operands[bakedCsg.transformedPrimitiveOperandIndices[t]],
                    bakedCsgs, scratch, ray, depthQueue, materialOverride, anyFound);
            }
        } else {
            for (long int t = bakedCsg.transformedPrimitiveOperandIndices.size() - 1;
                 t >= 0; t--) {
                const BakedScene::CsgOperandRecord &operand =
                    bakedCsg.operands[bakedCsg.transformedPrimitiveOperandIndices[t]];
                if (operand.kind == BakedScene::CsgOperandKind::TransformedQuadric &&
                    operand.bounded && operand.cullSafe &&
                    !AabbCullingSupport::rayIntersectsAabbForward(*ray, operand.bakedBounds)) {
                    continue;
                }
                dispatchTransformedPrimitiveOperand(
                    operand, bakedCsgs, scratch, ray, depthQueue, materialOverride, anyFound);
            }
        }

        return anyFound;
    }
};

#endif
