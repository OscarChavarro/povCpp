#ifndef __CSG_MORGAN_UNION_TRACE__
#define __CSG_MORGAN_UNION_TRACE__

#include "render/bakedScene/CsgContainmentTest.h"
#include "render/bakedScene/CsgOperandTrace.h"

class CsgMorganUnionTrace {
public:
    static int traceMorganCsg(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static int traceMorganIntersection(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static constexpr long int MAX_CACHED_QUADRIC_OPERANDS = 64;

    static inline int traceMorganIntersectionGeneric(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        const long int operandCount = bakedCsg->getOperands().size();
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
            const CsgOperandRecord *op = &bakedCsg->getOperands()[i];
            if (op->getKind() != BakedScene::CsgOperandKind::TransformedQuadric ||
                op->getQuadricGeometry() == nullptr) {
                continue;
            }
            const Vector3Dd localOrigin =
                op->getLocalToObject().transformPoint(ray->getOrigin());
            const Vector3Dd localDirection =
                op->getLocalToObject().transformDirection(ray->getDirection());
            double polyA = 0, polyB = 0, polyC = 0;
            bool trueMiss = false;
            double d1, d2;
            const bool hit = BakedQuadricIntersector::intersectBakedQuadricWithCoeffs(
                *op->getQuadricGeometry(), ray, localOrigin, localDirection,
                false, scratch.getCache(), op->getQuadricViewpointSlot(),
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

        for (long int i = bakedCsg->getOperands().size() - 1; i >= 0; i--) {
            const CsgOperandRecord *localShape = &bakedCsg->getOperands()[i];
            if (canCache && cachedValid[i]) {
                // Reuse the prescan's transform + quadric solve verbatim -
                // this mirrors CsgOperandTrace::traceOperandAllCrossings's
                // TransformedQuadric branch exactly (same effectiveMaterial
                // rule, same offerTransformedPrimitiveCandidate calls), just
                // fed from the cache instead of recomputing.
                if (cachedHit[i]) {
                    Material *effectiveMaterial =
                        localShape->getMaterial() != nullptr ? localShape->getMaterial() : materialOverride;
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
        const CsgOperandRecord *operand,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        bool &anyFound)
    {
        if (operand->getGeometry() == nullptr) {
            return;
        }
        Material *effectiveMaterial =
            operand->getMaterial() != nullptr ? operand->getMaterial() : materialOverride;
        if (operand->getKind() == BakedScene::CsgOperandKind::DirectAnnotatedPrimitive) {
            GeometryIntersectionEmissionContext context(
                effectiveMaterial, operand->getOperand(), true);
            if (operand->getGeometry()->doIntersectionForAllRayCrossingsAnnotated(
                    ray, depthQueue, context)) {
                anyFound = true;
            }
        } else {
            const int initialSize = depthQueue->size();
            if (operand->getGeometry()->doIntersectionForAllRayCrossings(
                    ray, depthQueue, effectiveMaterial) &&
                depthQueue->size() > initialSize) {
                if (CsgOperandTrace::annotateDirectCandidates(depthQueue, operand)) {
                    anyFound = true;
                }
            }
        }
    }

    static inline void dispatchTransformedPrimitiveOperand(
        const CsgOperandRecord *operand,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        bool &anyFound)
    {
        if (operand->getKind() == BakedScene::CsgOperandKind::TransformedQuadric) {
            if (operand->getGeometry() == nullptr || operand->getQuadricGeometry() == nullptr) {
                return;
            }
            Material *effectiveMaterial =
                operand->getMaterial() != nullptr ? operand->getMaterial() : materialOverride;
            const Vector3Dd localOrigin =
                operand->getLocalToObject().transformPoint(ray->getOrigin());
            const Vector3Dd localDirection =
                operand->getLocalToObject().transformDirection(ray->getDirection());
            double depth1;
            double depth2;
            if (!BakedQuadricIntersector::intersectBakedQuadric(
                    *operand->getQuadricGeometry(), ray, localOrigin, localDirection,
                    false, scratch.getCache(), operand->getQuadricViewpointSlot(),
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
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        bool anyFound = false;

        // Direct planes: inline emission, no dispatcher overhead.
        for (long int p = bakedCsg->getPlaneOperandIndices().size() - 1;
             p >= 0; p--) {
            const CsgOperandRecord *operand =
                &bakedCsg->getOperands()[bakedCsg->getPlaneOperandIndices()[p]];
            IntersectionCandidate candidate;
            if (BakedPlaneIntersector::tracePlaneOperandCandidate(
                    operand, ray, scratch.getCache(), materialOverride, candidate)) {
                depthQueue->offer(candidate);
                anyFound = true;
            }
        }

        // Direct primitives: bounds check then direct geometry call.
        for (long int d = bakedCsg->getDirectPrimitiveOperandIndices().size() - 1;
             d >= 0; d--) {
            const CsgOperandRecord *operand =
                &bakedCsg->getOperands()[bakedCsg->getDirectPrimitiveOperandIndices()[d]];
            if (operand->getGeometry() == nullptr) {
                continue;
            }
            if (operand->getBounded() && operand->getCullSafe() &&
                !operand->getBakedBounds().intersectsRayForward(*ray)) {
                continue;
            }
            Material *effectiveMaterial =
                operand->getMaterial() != nullptr ? operand->getMaterial() : materialOverride;
            if (operand->getKind() ==
                BakedScene::CsgOperandKind::DirectAnnotatedPrimitive) {
                GeometryIntersectionEmissionContext context(
                    effectiveMaterial, operand->getOperand(), true);
                if (operand->getGeometry()->doIntersectionForAllRayCrossingsAnnotated(
                        ray, depthQueue, context)) {
                    anyFound = true;
                }
            } else {
                const int initialSize = depthQueue->size();
                if (operand->getGeometry()->doIntersectionForAllRayCrossings(
                        ray, depthQueue, effectiveMaterial) &&
                    depthQueue->size() > initialSize) {
                    if (CsgOperandTrace::annotateDirectCandidates(depthQueue, operand)) {
                        anyFound = true;
                    }
                }
            }
        }

        // Nested CSGs: use compiled dispatch (handles compiled vs. generic per ray kind).
        for (long int n = bakedCsg->getNestedOperandIndices().size() - 1;
             n >= 0; n--) {
            if (CsgOperandTrace::tracePlanOperandAllCrossings(
                    &bakedCsg->getOperands()[bakedCsg->getNestedOperandIndices()[n]],
                    bakedCsgs,
                    scratch,
                    ray,
                    depthQueue,
                    materialOverride)) {
                anyFound = true;
            }
        }

        for (long int t = bakedCsg->getTransformedPrimitiveOperandIndices().size() - 1;
             t >= 0; t--) {
            const CsgOperandRecord *operand =
                &bakedCsg->getOperands()[bakedCsg->getTransformedPrimitiveOperandIndices()[t]];
            if (operand->getKind() == BakedScene::CsgOperandKind::TransformedQuadric) {
                if (operand->getGeometry() == nullptr || operand->getQuadricGeometry() == nullptr) {
                    continue;
                }
                if (operand->getBounded() && operand->getCullSafe() &&
                    !operand->getBakedBounds().intersectsRayForward(*ray)) {
                    continue;
                }
                Material *effectiveMaterial =
                    operand->getMaterial() != nullptr ? operand->getMaterial() : materialOverride;
                const Vector3Dd localOrigin =
                    operand->getLocalToObject().transformPoint(ray->getOrigin());
                const Vector3Dd localDirection =
                    operand->getLocalToObject().transformDirection(ray->getDirection());
                double depth1;
                double depth2;
                if (!BakedQuadricIntersector::intersectBakedQuadric(
                        *operand->getQuadricGeometry(), ray, localOrigin, localDirection,
                        false, scratch.getCache(), operand->getQuadricViewpointSlot(),
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
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        bool anyFound = false;

        for (long int p = bakedCsg->getPlaneOperandIndices().size() - 1;
             p >= 0; p--) {
            const CsgOperandRecord *operand =
                &bakedCsg->getOperands()[bakedCsg->getPlaneOperandIndices()[p]];
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
        if (bakedCsg->getDirectPrimitiveCullBins() != nullptr) {
            directCount = AabbCullingSupport::gatherCullSurvivors(
                *bakedCsg->getDirectPrimitiveCullBins(),
                bakedCsg->getDirectPrimitiveOperandIndices(),
                bakedCsg->getOperands(),
                *ray, directPositions, AabbCullingSupport::OPERAND_CULL_SCRATCH_CAPACITY);
            if (directCount >= 0) {
                AabbCullingSupport::sortPositionsDescending(directPositions, directCount);
            }
        }
        if (directCount >= 0) {
            for (int idx = 0; idx < directCount; idx++) {
                const long int d = directPositions[idx];
                dispatchDirectPrimitiveOperand(
                    &bakedCsg->getOperands()[bakedCsg->getDirectPrimitiveOperandIndices()[d]],
                    ray, depthQueue, materialOverride, anyFound);
            }
        } else {
            for (long int d = bakedCsg->getDirectPrimitiveOperandIndices().size() - 1;
                 d >= 0; d--) {
                const CsgOperandRecord *operand =
                    &bakedCsg->getOperands()[bakedCsg->getDirectPrimitiveOperandIndices()[d]];
                if (operand->getBounded() && operand->getCullSafe() &&
                    !operand->getBakedBounds().intersectsRayForward(*ray)) {
                    continue;
                }
                dispatchDirectPrimitiveOperand(
                    operand, ray, depthQueue, materialOverride, anyFound);
            }
        }

        for (long int n = bakedCsg->getNestedOperandIndices().size() - 1;
             n >= 0; n--) {
            if (CsgOperandTrace::tracePlanOperandAllCrossings(
                    &bakedCsg->getOperands()[bakedCsg->getNestedOperandIndices()[n]],
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
        if (bakedCsg->getTransformedPrimitiveCullBins() != nullptr) {
            transformedCount = AabbCullingSupport::gatherCullSurvivors(
                *bakedCsg->getTransformedPrimitiveCullBins(),
                bakedCsg->getTransformedPrimitiveOperandIndices(),
                bakedCsg->getOperands(),
                *ray, transformedPositions, AabbCullingSupport::OPERAND_CULL_SCRATCH_CAPACITY);
            if (transformedCount >= 0) {
                AabbCullingSupport::sortPositionsDescending(transformedPositions, transformedCount);
            }
        }
        if (transformedCount >= 0) {
            for (int idx = 0; idx < transformedCount; idx++) {
                const long int t = transformedPositions[idx];
                dispatchTransformedPrimitiveOperand(
                    &bakedCsg->getOperands()[bakedCsg->getTransformedPrimitiveOperandIndices()[t]],
                    bakedCsgs, scratch, ray, depthQueue, materialOverride, anyFound);
            }
        } else {
            for (long int t = bakedCsg->getTransformedPrimitiveOperandIndices().size() - 1;
                 t >= 0; t--) {
                const CsgOperandRecord *operand =
                    &bakedCsg->getOperands()[bakedCsg->getTransformedPrimitiveOperandIndices()[t]];
                if (operand->getKind() == BakedScene::CsgOperandKind::TransformedQuadric &&
                    operand->getBounded() && operand->getCullSafe() &&
                    !operand->getBakedBounds().intersectsRayForward(*ray)) {
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
