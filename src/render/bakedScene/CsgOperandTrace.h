#ifndef __CSG_OPERAND_TRACE__
#define __CSG_OPERAND_TRACE__

#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "render/bakedScene/AabbCullingSupport.h"
#include "render/bakedScene/BakedPlaneIntersector.h"
#include "render/bakedScene/BakedQuadricIntersector.h"
#include "render/bakedScene/CsgScratchContext.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

// Generic per-operand "all crossings" dispatch engine: the fallback path
// that walks every BakedScene::CsgOperandKind directly (used whenever an
// operand does not qualify for one of the compiled fast paths in
// SingleCorePlaneCsgTrace or CsgMorganUnionTrace), plus the top-level
// traceAllCrossings entry point.
class CsgOperandTrace {
public:
    static bool traceAllCrossings(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache,
        Material *materialOverride = nullptr);

    static bool traceAllCrossingsWithScratch(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool tracePlanOperandAllCrossings(
        const CsgOperandRecord *operand,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static void offerTransformedPrimitiveCandidate(
        const CsgOperandRecord *operand,
        RayWithTracingState *ray,
        Material *effectiveMaterial,
        const Vector3Dd &localOrigin,
        const Vector3Dd &localDirection,
        double depth,
        java::PriorityQueue<IntersectionCandidate> *depthQueue)
    {
        IntersectionCandidate candidate;
        candidate.getIntersection().point =
            localOrigin.add(localDirection.multiply(depth));
        candidate.getAttributes().setHitGeometry(operand->getGeometry());
        candidate.getAttributes().setMaterial(effectiveMaterial);
        candidate.getAttributes().pushDetailOwner(operand->getOperand());
        candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
        candidate.getIntersection().point =
            operand->getObjectToLocal().transformPoint(candidate.getIntersection().point);
        const Vector3Dd rayOrigin = ray->getOrigin();
        const Vector3Dd rayDir = ray->getDirection();
        candidate.getIntersection().t =
            candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDir) /
            rayDir.dotProduct(rayDir);
        depthQueue->offer(candidate);
    }

    static bool annotateDirectCandidates(
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const CsgOperandRecord *operand)
    {
        bool annotated = false;
        // The direct-to-destination fast path is only used for non-transformed,
        // non-nested primitive geometry. Existing queue entries have already been
        // annotated by this helper; fresh primitive hits are the only ones still
        // carrying an empty detail-owner stack and the default point-space flag.
        for (IntersectionCandidate &candidate : *depthQueue) {
            IntersectionAttributes &attributes = candidate.getAttributes();
            if (attributes.getDetailOwnerCount() != 0 ||
                attributes.getMaterialUsesObjectLocalPoint()) {
                continue;
            }
            attributes.pushDetailOwner(operand->getOperand());
            attributes.setMaterialUsesObjectLocalPoint(true);
            annotated = true;
        }
        return annotated;
    }

    static inline bool traceOperandAllCrossings(
        const CsgOperandRecord *operand,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        if (operand->getGeometry() == nullptr) {
            return false;
        }
        if (operand->getBounded() && operand->getCullSafe() &&
            !operand->getBakedBounds().intersectsRayForward(*ray)) {
            return false;
        }

        Material *effectiveMaterial =
            operand->getMaterial() != nullptr ? operand->getMaterial() : materialOverride;
        switch (operand->getKind()) {
        case BakedScene::CsgOperandKind::DirectAnnotatedPrimitive:
        {
            GeometryIntersectionEmissionContext context(
                effectiveMaterial, operand->getOperand(), true);
            return operand->getGeometry()->doIntersectionForAllRayCrossingsAnnotated(
                ray, depthQueue, context);
        }

        case BakedScene::CsgOperandKind::DirectPrimitive: {
            const int initialSize = depthQueue->size();
            const bool found = operand->getGeometry()->doIntersectionForAllRayCrossings(
                ray, depthQueue, effectiveMaterial);
            if (!found || depthQueue->size() == initialSize) {
                return false;
            }
            return annotateDirectCandidates(depthQueue, operand);
        }

        default:
            break;
        }

        RayWithTracingState *localRayPtr = ray;
        if (operand->getKind() ==
                BakedScene::CsgOperandKind::TransformedPlane ||
            operand->getKind() ==
                BakedScene::CsgOperandKind::TransformedQuadric ||
            operand->getKind() ==
                BakedScene::CsgOperandKind::TransformedSphere ||
            operand->getKind() ==
                BakedScene::CsgOperandKind::TransformedPrimitive ||
            operand->getKind() ==
                BakedScene::CsgOperandKind::TransformedNestedCsg) {
            if (operand->getKind() ==
                BakedScene::CsgOperandKind::TransformedPlane) {
                const Vector3Dd localOrigin =
                    operand->getLocalToObject().transformPoint(ray->getOrigin());
                const Vector3Dd localDirection =
                    operand->getLocalToObject().transformDirection(ray->getDirection());

                double depth;
                if (!BakedPlaneIntersector::intersectBakedPlane(
                        operand,
                        ray,
                        localOrigin,
                        localDirection,
                        scratch.getCache(),
                        &depth) ||
                    depth <= GeometryConfig::SMALL_TOLERANCE) {
                    return false;
                }

                IntersectionCandidate candidate;
                candidate.getIntersection().point =
                    localOrigin.add(localDirection.multiply(depth));
                candidate.getAttributes().setHitGeometry(operand->getGeometry());
                candidate.getAttributes().setMaterial(effectiveMaterial);
                candidate.getAttributes().pushDetailOwner(operand->getOperand());
                candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
                candidate.getIntersection().point =
                    operand->getObjectToLocal().transformPoint(candidate.getIntersection().point);
                const Vector3Dd rayOrigin = ray->getOrigin();
                const Vector3Dd rayDir = ray->getDirection();
                candidate.getIntersection().t =
                    candidate.getIntersection().point
                        .subtract(rayOrigin).dotProduct(rayDir) /
                    rayDir.dotProduct(rayDir);
                depthQueue->offer(candidate);
                return true;
            }

            if (operand->getKind() ==
                BakedScene::CsgOperandKind::TransformedQuadric) {
                const Vector3Dd localOrigin =
                    operand->getLocalToObject().transformPoint(ray->getOrigin());
                const Vector3Dd localDirection =
                    operand->getLocalToObject().transformDirection(ray->getDirection());

                double depth1;
                double depth2;
                if (operand->getQuadricGeometry() == nullptr) {
                    return false;
                }
                if (!BakedQuadricIntersector::intersectBakedQuadric(
                        *operand->getQuadricGeometry(),
                        ray,
                        localOrigin,
                        localDirection,
                        false,
                        scratch.getCache(),
                        operand->getQuadricViewpointSlot(),
                        &depth1,
                        &depth2)) {
                    return false;
                }

                offerTransformedPrimitiveCandidate(
                    operand,
                    ray,
                    effectiveMaterial,
                    localOrigin,
                    localDirection,
                    depth1,
                    depthQueue);
                if (depth2 != depth1) {
                    offerTransformedPrimitiveCandidate(
                        operand,
                        ray,
                        effectiveMaterial,
                        localOrigin,
                        localDirection,
                        depth2,
                        depthQueue);
                }
                return true;
            }

            if (operand->getKind() ==
                BakedScene::CsgOperandKind::TransformedSphere) {
                const Vector3Dd localOrigin =
                    operand->getLocalToObject().transformPoint(ray->getOrigin());
                const Vector3Dd localDirection =
                    operand->getLocalToObject().transformDirection(ray->getDirection());

                double depth1;
                double depth2;
                if (!Sphere::intersectSphereLocalSpace(
                        localOrigin, localDirection, ray->getGeometryStatistics(),
                        static_cast<Sphere *>(operand->getGeometry())->getRadius(),
                        &depth1, &depth2)) {
                    return false;
                }

                offerTransformedPrimitiveCandidate(
                    operand, ray, effectiveMaterial,
                    localOrigin, localDirection, depth1, depthQueue);
                if (depth2 != depth1) {
                    offerTransformedPrimitiveCandidate(
                        operand, ray, effectiveMaterial,
                        localOrigin, localDirection, depth2, depthQueue);
                }
                return true;
            }

            RayWithTracingState localRay = RayWithTracingState::localIntersectionClone(*ray);
            localRay.setOrigin(operand->getLocalToObject().transformPoint(ray->getOrigin()));
            localRay.setDirection(operand->getLocalToObject().transformDirection(ray->getDirection()));
            localRay.setQuadricConstantsCached(false);
            localRayPtr = &localRay;

            java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
                scratch.borrowQueue();

            bool found = false;
            if (operand->getKind() ==
                BakedScene::CsgOperandKind::TransformedNestedCsg) {
                found = traceAllCrossingsWithScratch(
                    bakedCsgs[operand->getNestedCsgProgramIndex()],
                    bakedCsgs,
                    scratch,
                    localRayPtr,
                    localDepthQueue,
                    effectiveMaterial);
            } else {
                found = operand->getGeometry()->doIntersectionForAllRayCrossings(
                    localRayPtr, localDepthQueue, effectiveMaterial);
            }

            const Vector3Dd rayOrigin = ray->getOrigin();
            const Vector3Dd rayDir = ray->getDirection();
            const double dirLenSq = rayDir.dotProduct(rayDir);
            for (IntersectionCandidate &candidate : *localDepthQueue) {
                candidate.getAttributes().pushDetailOwner(operand->getOperand());
                candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
                candidate.getIntersection().point =
                    operand->getObjectToLocal().transformPoint(candidate.getIntersection().point);
                candidate.getIntersection().t =
                    candidate.getIntersection().point
                        .subtract(rayOrigin).dotProduct(rayDir) / dirLenSq;
                depthQueue->offer(candidate);
            }

            scratch.returnQueue(localDepthQueue);
            return found;
        }

        if (operand->getKind() == BakedScene::CsgOperandKind::DirectPlane) {
            double depth;
            if (!BakedPlaneIntersector::intersectBakedPlane(
                    operand,
                    ray,
                    ray->getOrigin(),
                    ray->getDirection(),
                    scratch.getCache(),
                    &depth) ||
                depth <= GeometryConfig::SMALL_TOLERANCE) {
                return false;
            }

            IntersectionCandidate candidate;
            candidate.getIntersection().point =
                ray->getOrigin().add(ray->getDirection().multiply(depth));
            candidate.getAttributes().setHitGeometry(operand->getGeometry());
            candidate.getAttributes().setMaterial(effectiveMaterial);
            candidate.getAttributes().pushDetailOwner(operand->getOperand());
            candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
            candidate.getIntersection().t = depth;
            depthQueue->offer(candidate);
            return true;
        }

        const Vector3Dd rayOrigin = ray->getOrigin();
        const Vector3Dd rayDir = ray->getDirection();
        const double dirLenSq = rayDir.dotProduct(rayDir);
        java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
            scratch.borrowQueue();
        bool found = false;
        if (operand->getKind() == BakedScene::CsgOperandKind::NestedCsg) {
            found = traceAllCrossingsWithScratch(
                bakedCsgs[operand->getNestedCsgProgramIndex()],
                bakedCsgs,
                scratch,
                ray,
                localDepthQueue,
                effectiveMaterial);
        } else {
            found = operand->getGeometry()->doIntersectionForAllRayCrossings(
                ray, localDepthQueue, effectiveMaterial);
        }
        for (IntersectionCandidate &candidate : *localDepthQueue) {
            candidate.getAttributes().pushDetailOwner(operand->getOperand());
            candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
            candidate.getIntersection().t =
                candidate.getIntersection().point
                    .subtract(rayOrigin).dotProduct(rayDir) / dirLenSq;
            depthQueue->offer(candidate);
        }

        scratch.returnQueue(localDepthQueue);
        return found;
    }
};

#endif
