#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "common/Config.h"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/volume/Quadric.h"
#include "render/bakedScene/BakedCsgTracing.h"
#include "render/bakedScene/BakedSimpleBodyTracing.h"
#include "render/bakedScene/BakedTracingCommon.h"

bool
BakedSimpleBodyTracing::finalizeCandidate(
    const Scene::BakedSimpleBody &baked,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    RayWithSegments *ray,
    IntersectionCandidate &candidate)
{
    if (baked.hasGeometryTransform) {
        candidate.getIntersection().point =
            baked.geometryToObject.transformPoint(candidate.getIntersection().point);
    }
    const Vector3Dd objectLocalPoint = candidate.getIntersection().point;

    if (baked.hasClippingShapes) {
        for (long int i = baked.clippingObjects.size() - 1; i >= 0; i--) {
            if (BakedTracingCommon::containmentTest(
                    baked.clippingObjects[i],
                    bakedSimpleBodies,
                    bakedCsgs,
                    bakedComposites,
                    objectLocalPoint,
                    Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
                return false;
            }
        }
    }

    candidate.getAttributes().setObjectTexture(baked.objectTexture);
    candidate.getAttributes().setObjectColor(baked.objectColor);
    candidate.getAttributes().setNoShadowFlag(baked.noShadowFlag);
    candidate.getAttributes().setHitBody(baked.object);
    if (baked.hasObjectTransform) {
        candidate.getIntersection().point =
            baked.objectToWorld.transformPoint(candidate.getIntersection().point);
    }

    const Vector3Dd rayDir = ray->getDirection();
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(ray->getOrigin()).dotProduct(rayDir) /
        rayDir.dotProduct(rayDir);
    return true;
}

bool
BakedSimpleBodyTracing::passesBoundingShapes(
    const Scene::BakedSimpleBody &baked,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    RayWithSegments *objectRayPtr)
{
    for (long int i = baked.boundingObjects.size() - 1; i >= 0; i--) {
        IntersectionCandidate boundingHit;
        const Vector3Dd objectRayOrigin = objectRayPtr->getOrigin();
        if (!BakedTracingCommon::traceObjectFirstHit(
                baked.boundingObjects[i],
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
                objectRayPtr,
                boundingHit) &&
            BakedTracingCommon::containmentTest(
                baked.boundingObjects[i],
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
                objectRayOrigin,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
BakedSimpleBodyTracing::traceFirstHit(
    const Scene::BakedSimpleBody &baked,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    RayWithSegments *ray,
    IntersectionCandidate &out)
{
    if (baked.geometry == nullptr) {
        return false;
    }

    const bool canUseGeometryFirstHit =
        baked.executionKind == Scene::BakedSimpleBodyExecutionKind::DirectPrimitive ||
        baked.executionKind == Scene::BakedSimpleBodyExecutionKind::TransformedPrimitive;

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
    if (canUseGeometryFirstHit) {
        IntersectionCandidate candidate;
        if (baked.geometry->doIntersectionFirstHitNoQueue(
                geometryRayPtr,
                candidate,
                baked.geometryMaterial)) {
            if (!finalizeCandidate(
                    baked,
                    bakedSimpleBodies,
                    bakedCsgs,
                    bakedComposites,
                    ray,
                    candidate)) {
                return false;
            }
            out = candidate;
            return true;
        }
    }

    if (baked.bakedCsgIndex >= 0) {
        IntersectionCandidate candidate;
        if (!BakedCsgTracing::traceFirstHit(
                bakedCsgs[baked.bakedCsgIndex],
                bakedCsgs,
                geometryRayPtr,
                candidate,
                baked.geometryMaterial)) {
            return false;
        }
        if (!finalizeCandidate(
                baked,
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
                ray,
                candidate)) {
            return false;
        }
        out = candidate;
        return true;
    }

    java::PriorityQueue<IntersectionCandidate> * const depthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    const bool hasHit = baked.bakedCsgIndex >= 0 ?
        (BakedCsgTracing::traceAllCrossings(
             bakedCsgs[baked.bakedCsgIndex],
             bakedCsgs,
             geometryRayPtr,
             depthQueue,
             baked.geometryMaterial) &&
         depthQueue->size() > 0) :
        (baked.geometry->doIntersectionForAllRayCrossings(
             geometryRayPtr, depthQueue, baked.geometryMaterial) &&
         depthQueue->size() > 0);
    if (!hasHit) {
        ray->getIntersectionQueuePool()->push(depthQueue);
        return false;
    }

    bool found = false;
    if (!baked.hasClippingShapes) {
        IntersectionCandidate nearest = depthQueue->peek();
        if (finalizeCandidate(
                baked,
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
                ray,
                nearest)) {
            out = nearest;
            found = true;
        }
    } else {
        while (depthQueue->size() > 0) {
            IntersectionCandidate candidate = depthQueue->poll();
            if (finalizeCandidate(
                    baked,
                    bakedSimpleBodies,
                    bakedCsgs,
                    bakedComposites,
                    ray,
                    candidate)) {
                out = candidate;
                found = true;
                break;
            }
        }
    }

    depthQueue->clear();
    ray->getIntersectionQueuePool()->push(depthQueue);
    return found;
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

int
BakedSimpleBodyTracing::containmentTest(
    const Scene::BakedSimpleBody &baked,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    const Vector3Dd &point,
    double distanceTolerance)
{
    Vector3Dd localPoint = point;
    if (baked.hasObjectTransform) {
        localPoint = baked.worldToObject.transformPoint(point);
    }

    for (long int i = baked.boundingObjects.size() - 1; i >= 0; i--) {
        if (BakedTracingCommon::containmentTest(
                baked.boundingObjects[i],
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
                localPoint,
                distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    for (long int i = baked.clippingObjects.size() - 1; i >= 0; i--) {
        if (BakedTracingCommon::containmentTest(
                baked.clippingObjects[i],
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
                localPoint,
                distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    if (baked.geometry == nullptr) {
        return Geometry::OUTSIDE;
    }

    Vector3Dd geometryPoint = localPoint;
    if (baked.hasGeometryTransform) {
        geometryPoint = baked.objectToGeometry.transformPoint(localPoint);
    }
    const int containment = baked.bakedCsgIndex >= 0 ?
        BakedCsgTracing::containmentTest(
            bakedCsgs[baked.bakedCsgIndex],
            bakedCsgs,
            geometryPoint,
            distanceTolerance) :
        baked.geometry->doContainmentTest(
            geometryPoint,
            distanceTolerance);
    if (containment != Geometry::OUTSIDE) {
        return Geometry::INSIDE;
    }
    return Geometry::OUTSIDE;
}
