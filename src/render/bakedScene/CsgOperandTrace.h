#ifndef __CSG_OPERAND_TRACE__
#define __CSG_OPERAND_TRACE__

#include "common/Config.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "environment/material/Material.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "render/bakedScene/AabbCullingSupport.h"
#include "render/bakedScene/BakedPlaneIntersector.h"
#include "render/bakedScene/BakedQuadricIntersector.h"
#include "render/bakedScene/BakedScene.h"
#include "render/bakedScene/CsgScratchContext.h"
#include "render/raySharedCache/RaySharedCache.h"

// Generic per-operand "all crossings" dispatch engine: the fallback path
// that walks every BakedScene::CsgOperandKind directly (used whenever an
// operand does not qualify for one of the compiled fast paths in
// SingleCorePlaneCsgTrace or CsgMorganUnionTrace), plus the top-level
// traceAllCrossings entry point.
class CsgOperandTrace {
public:
    static bool traceAllCrossings(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache,
        Material *materialOverride = nullptr);

    static bool traceAllCrossingsWithScratch(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool tracePlanOperandAllCrossings(
        const BakedScene::CsgOperandRecord &operand,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static void offerTransformedPrimitiveCandidate(
        const BakedScene::CsgOperandRecord &operand,
        RayWithSegments *ray,
        Material *effectiveMaterial,
        const Vector3Dd &localOrigin,
        const Vector3Dd &localDirection,
        double depth,
        java::PriorityQueue<IntersectionCandidate> *depthQueue)
    {
        IntersectionCandidate candidate;
        candidate.getIntersection().point =
            localOrigin.add(localDirection.multiply(depth));
        candidate.getAttributes().setHitGeometry(operand.geometry);
        candidate.getAttributes().setMaterial(effectiveMaterial);
        candidate.getAttributes().pushDetailOwner(operand.operand);
        candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
        candidate.getIntersection().point =
            operand.objectToLocal.transformPoint(candidate.getIntersection().point);
        const Vector3Dd rayOrigin = ray->getOrigin();
        const Vector3Dd rayDir = ray->getDirection();
        candidate.getIntersection().t =
            candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDir) /
            rayDir.dotProduct(rayDir);
        depthQueue->offer(candidate);
    }

    static bool annotateDirectCandidates(
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const BakedScene::CsgOperandRecord &operand)
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
            attributes.pushDetailOwner(operand.operand);
            attributes.setMaterialUsesObjectLocalPoint(true);
            annotated = true;
        }
        return annotated;
    }

    static inline bool traceOperandAllCrossings(
        const BakedScene::CsgOperandRecord &operand,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        if (operand.geometry == nullptr) {
            return false;
        }
        if (operand.bounded && operand.cullSafe &&
            !AabbCullingSupport::rayIntersectsAabbForward(*ray, operand.bakedBounds)) {
            return false;
        }

        Material *effectiveMaterial =
            operand.material != nullptr ? operand.material : materialOverride;
        switch (operand.kind) {
        case BakedScene::CsgOperandKind::DirectAnnotatedPrimitive:
        {
            GeometryIntersectionEmissionContext context;
            context.materialOverride = effectiveMaterial;
            context.detailOwner = operand.operand;
            context.materialUsesObjectLocalPoint = true;
            return operand.geometry->doIntersectionForAllRayCrossingsAnnotated(
                ray, depthQueue, context);
        }

        case BakedScene::CsgOperandKind::DirectPrimitive: {
            const int initialSize = depthQueue->size();
            const bool found = operand.geometry->doIntersectionForAllRayCrossings(
                ray, depthQueue, effectiveMaterial);
            if (!found || depthQueue->size() == initialSize) {
                return false;
            }
            return annotateDirectCandidates(depthQueue, operand);
        }

        default:
            break;
        }

        RayWithSegments *localRayPtr = ray;
        if (operand.kind ==
                BakedScene::CsgOperandKind::TransformedPlane ||
            operand.kind ==
                BakedScene::CsgOperandKind::TransformedQuadric ||
            operand.kind ==
                BakedScene::CsgOperandKind::TransformedSphere ||
            operand.kind ==
                BakedScene::CsgOperandKind::TransformedPrimitive ||
            operand.kind ==
                BakedScene::CsgOperandKind::TransformedNestedCsg) {
            if (operand.kind ==
                BakedScene::CsgOperandKind::TransformedPlane) {
                const Vector3Dd localOrigin =
                    operand.localToObject.transformPoint(ray->getOrigin());
                const Vector3Dd localDirection =
                    operand.localToObject.transformDirection(ray->getDirection());

                double depth;
                if (!BakedPlaneIntersector::intersectBakedPlane(
                        operand,
                        ray,
                        localOrigin,
                        localDirection,
                        scratch.getCache(),
                        &depth) ||
                    depth <= Config::SMALL_TOLERANCE) {
                    return false;
                }

                IntersectionCandidate candidate;
                candidate.getIntersection().point =
                    localOrigin.add(localDirection.multiply(depth));
                candidate.getAttributes().setHitGeometry(operand.geometry);
                candidate.getAttributes().setMaterial(effectiveMaterial);
                candidate.getAttributes().pushDetailOwner(operand.operand);
                candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
                candidate.getIntersection().point =
                    operand.objectToLocal.transformPoint(candidate.getIntersection().point);
                const Vector3Dd rayOrigin = ray->getOrigin();
                const Vector3Dd rayDir = ray->getDirection();
                candidate.getIntersection().t =
                    candidate.getIntersection().point
                        .subtract(rayOrigin).dotProduct(rayDir) /
                    rayDir.dotProduct(rayDir);
                depthQueue->offer(candidate);
                return true;
            }

            if (operand.kind ==
                BakedScene::CsgOperandKind::TransformedQuadric) {
                const Vector3Dd localOrigin =
                    operand.localToObject.transformPoint(ray->getOrigin());
                const Vector3Dd localDirection =
                    operand.localToObject.transformDirection(ray->getDirection());

                double depth1;
                double depth2;
                if (operand.quadricGeometry == nullptr) {
                    return false;
                }
                if (!BakedQuadricIntersector::intersectBakedQuadric(
                        *operand.quadricGeometry,
                        ray,
                        localOrigin,
                        localDirection,
                        false,
                        scratch.getCache(),
                        operand.quadricViewpointSlot,
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

            if (operand.kind ==
                BakedScene::CsgOperandKind::TransformedSphere) {
                const Vector3Dd localOrigin =
                    operand.localToObject.transformPoint(ray->getOrigin());
                const Vector3Dd localDirection =
                    operand.localToObject.transformDirection(ray->getDirection());

                double depth1;
                double depth2;
                if (!Sphere::intersectSphereLocalSpace(
                        localOrigin, localDirection,
                        ray->getStatistics(), &depth1, &depth2)) {
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

            RayWithSegments localRay(RayWithSegments::LocalIntersectionClone{}, *ray);
            localRay.setOrigin(operand.localToObject.transformPoint(ray->getOrigin()));
            localRay.setDirection(operand.localToObject.transformDirection(ray->getDirection()));
            localRay.setQuadricConstantsCached(false);
            localRayPtr = &localRay;

            java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
                scratch.borrowQueue();

            bool found = false;
            if (operand.kind ==
                BakedScene::CsgOperandKind::TransformedNestedCsg) {
                found = traceAllCrossingsWithScratch(
                    bakedCsgs[operand.nestedCsgProgramIndex],
                    bakedCsgs,
                    scratch,
                    localRayPtr,
                    localDepthQueue,
                    effectiveMaterial);
            } else {
                found = operand.geometry->doIntersectionForAllRayCrossings(
                    localRayPtr, localDepthQueue, effectiveMaterial);
            }

            const Vector3Dd rayOrigin = ray->getOrigin();
            const Vector3Dd rayDir = ray->getDirection();
            const double dirLenSq = rayDir.dotProduct(rayDir);
            for (IntersectionCandidate &candidate : *localDepthQueue) {
                candidate.getAttributes().pushDetailOwner(operand.operand);
                candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
                candidate.getIntersection().point =
                    operand.objectToLocal.transformPoint(candidate.getIntersection().point);
                candidate.getIntersection().t =
                    candidate.getIntersection().point
                        .subtract(rayOrigin).dotProduct(rayDir) / dirLenSq;
                depthQueue->offer(candidate);
            }

            scratch.returnQueue(localDepthQueue);
            return found;
        }

        if (operand.kind == BakedScene::CsgOperandKind::DirectPlane) {
            double depth;
            if (!BakedPlaneIntersector::intersectBakedPlane(
                    operand,
                    ray,
                    ray->getOrigin(),
                    ray->getDirection(),
                    scratch.getCache(),
                    &depth) ||
                depth <= Config::SMALL_TOLERANCE) {
                return false;
            }

            IntersectionCandidate candidate;
            candidate.getIntersection().point =
                ray->getOrigin().add(ray->getDirection().multiply(depth));
            candidate.getAttributes().setHitGeometry(operand.geometry);
            candidate.getAttributes().setMaterial(effectiveMaterial);
            candidate.getAttributes().pushDetailOwner(operand.operand);
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
        if (operand.kind == BakedScene::CsgOperandKind::NestedCsg) {
            found = traceAllCrossingsWithScratch(
                bakedCsgs[operand.nestedCsgProgramIndex],
                bakedCsgs,
                scratch,
                ray,
                localDepthQueue,
                effectiveMaterial);
        } else {
            found = operand.geometry->doIntersectionForAllRayCrossings(
                ray, localDepthQueue, effectiveMaterial);
        }
        for (IntersectionCandidate &candidate : *localDepthQueue) {
            candidate.getAttributes().pushDetailOwner(operand.operand);
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
