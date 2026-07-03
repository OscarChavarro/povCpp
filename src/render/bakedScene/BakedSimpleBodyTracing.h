#ifndef __BAKED_SIMPLE_BODY_TRACING__
#define __BAKED_SIMPLE_BODY_TRACING__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/scene/Scene.h"
#include "common/Config.h"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/volume/Quadric.h"
#include "render/bakedScene/BakedCsgTracing.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

class BakedSimpleBodyTracing {
public:
    static bool traceFirstHit(
        const Scene::BakedSimpleBody &baked,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        RayWithSegments *ray,
        IntersectionCandidate &out);

    static bool traceAllCrossings(
        const Scene::BakedSimpleBody &baked,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue)
    {
        if (baked.geometry == nullptr) {
            return false;
        }

        // Fast path: direct quadric simple body with no bounding or clipping shapes
        // and no nested CSG. Computes transforms and intersection inline, avoiding
        // both LocalIntersectionClone and the localDepthQueue pool allocation.
        if (!baked.hasBoundingShapes && !baked.hasClippingShapes &&
            baked.bakedCsgIndex < 0 && baked.quadricGeometry != nullptr) {
            const Vector3Dd worldOrigin = ray->getOrigin();
            const Vector3Dd worldDir = ray->getDirection();
            const Vector3Dd localOrigin = baked.hasObjectTransform ?
                baked.worldToObject.transformPoint(worldOrigin) : worldOrigin;
            const Vector3Dd localDir = baked.hasObjectTransform ?
                baked.worldToObject.transformDirection(worldDir) : worldDir;
            const Vector3Dd geomOrigin = baked.hasGeometryTransform ?
                baked.objectToGeometry.transformPoint(localOrigin) : localOrigin;
            const Vector3Dd geomDir = baked.hasGeometryTransform ?
                baked.objectToGeometry.transformDirection(localDir) : localDir;

            double depth1;
            double depth2;
            if (!BakedCsgTracing::intersectBakedQuadric(
                    *baked.quadricGeometry, ray, geomOrigin, geomDir, &depth1, &depth2)) {
                return false;
            }

            const auto offerQuadricCandidate = [&](double depth) {
                IntersectionCandidate candidate;
                candidate.getIntersection().point = geomOrigin.add(geomDir.multiply(depth));
                candidate.getAttributes().setHitGeometry(baked.quadricGeometry);
                candidate.getAttributes().setMaterial(baked.geometryMaterial);
                if (baked.hasGeometryTransform) {
                    candidate.getIntersection().point =
                        baked.geometryToObject.transformPoint(candidate.getIntersection().point);
                }
                candidate.getAttributes().setObjectTexture(baked.objectTexture);
                candidate.getAttributes().setObjectColor(baked.objectColor);
                candidate.getAttributes().setNoShadowFlag(baked.noShadowFlag);
                candidate.getAttributes().setHitBody(baked.object);
                if (baked.hasObjectTransform) {
                    candidate.getIntersection().point =
                        baked.objectToWorld.transformPoint(candidate.getIntersection().point);
                }
                candidate.getIntersection().t =
                    candidate.getIntersection().point
                        .subtract(worldOrigin).dotProduct(worldDir) /
                    worldDir.dotProduct(worldDir);
                depthQueue->offer(candidate);
            };

            offerQuadricCandidate(depth1);
            if (depth2 != depth1) {
                offerQuadricCandidate(depth2);
            }
            return true;
        }

        auto traceInObjectSpace = [&](RayWithSegments *objectRayPtr) -> bool {

        if (!passesBoundingShapes(
                baked,
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
                objectRayPtr)) {
            return false;
        }

        auto traceInGeometrySpace = [&](RayWithSegments *geometryRayPtr) -> bool {
        java::PriorityQueue<IntersectionCandidate> * const localDepthQueue =
            ray->getIntersectionQueuePool()->pop(128);
        const bool foundAny = baked.bakedCsgIndex >= 0 ?
            (BakedCsgTracing::traceAllCrossings(
                 bakedCsgs[baked.bakedCsgIndex],
                 bakedCsgs,
                 geometryRayPtr,
                 localDepthQueue,
                 baked.geometryMaterial) &&
             localDepthQueue->size() > 0) :
            (baked.geometry->doIntersectionForAllRayCrossings(
                 geometryRayPtr, localDepthQueue, baked.geometryMaterial) &&
             localDepthQueue->size() > 0);
        if (!foundAny) {
            ray->getIntersectionQueuePool()->push(localDepthQueue);
            return false;
        }

        bool accepted = false;
        for (IntersectionCandidate &candidate : *localDepthQueue) {
            if (finalizeCandidate(
                    baked,
                    bakedSimpleBodies,
                    bakedCsgs,
                    bakedComposites,
                    ray,
                    candidate)) {
                depthQueue->offer(candidate);
                accepted = true;
            }
        }

        localDepthQueue->clear();
        ray->getIntersectionQueuePool()->push(localDepthQueue);
        return accepted;
        };

        if (baked.hasGeometryTransform) {
            RayWithSegments geometryRay(
                RayWithSegments::LocalIntersectionClone{}, *objectRayPtr);
            geometryRay.setOrigin(
                baked.objectToGeometry.transformPoint(objectRayPtr->getOrigin()));
            geometryRay.setDirection(
                baked.objectToGeometry.transformDirection(objectRayPtr->getDirection()));
            geometryRay.setQuadricConstantsCached(false);
            return traceInGeometrySpace(&geometryRay);
        }
        return traceInGeometrySpace(objectRayPtr);
        };

        if (baked.hasObjectTransform) {
            RayWithSegments objectRay(
                RayWithSegments::LocalIntersectionClone{}, *ray);
            objectRay.setOrigin(baked.worldToObject.transformPoint(ray->getOrigin()));
            objectRay.setDirection(baked.worldToObject.transformDirection(ray->getDirection()));
            objectRay.setQuadricConstantsCached(false);
            return traceInObjectSpace(&objectRay);
        }
        return traceInObjectSpace(ray);
    }

    static int containmentTest(
        const Scene::BakedSimpleBody &baked,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        const Vector3Dd &point,
        double distanceTolerance);

private:
    static bool finalizeCandidate(
        const Scene::BakedSimpleBody &baked,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        RayWithSegments *ray,
        IntersectionCandidate &candidate);

    static bool passesBoundingShapes(
        const Scene::BakedSimpleBody &baked,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        RayWithSegments *objectRayPtr);
};

#endif
