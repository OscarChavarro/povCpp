#ifndef __CSG_MORGAN_UNION_TRACE__
#define __CSG_MORGAN_UNION_TRACE__

#include "common/Config.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithTracingState.h"
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

class CsgMorganUnionTrace {
public:
    static int traceMorganCsg(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static int traceMorganIntersection(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static constexpr long int MAX_CACHED_QUADRIC_OPERANDS = 64;

    static inline int traceMorganIntersectionGeneric(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
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

    static inline void dispatchDirectPrimitiveOperand(
        const BakedScene::CsgOperandRecord &operand,
        RayWithTracingState *ray,
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

    static inline void dispatchTransformedPrimitiveOperand(
        const BakedScene::CsgOperandRecord &operand,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
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

    static inline bool traceGenericMorganUnion(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
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

    static inline bool traceGenericMorganUnionWithCullBins(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
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
