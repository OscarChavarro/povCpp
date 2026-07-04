#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "common/Config.h"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/volume/Quadric.h"
#include "environment/scene/Composite.h"
#include "render/bakedScene/BakedCsgTrace.h"
#include "render/bakedScene/BakedTrace.h"

// Uses RayWithSegments's cached per-axis reciprocals (Plan 12 Phase 3 - see
// BakedCsgTrace::rayIntersectsAabbForward for the identical pattern and
// rationale) instead of dividing on every call.
bool
BakedTrace::rayIntersectsAabbForward(const RayWithSegments &ray, const AxisAlignedBoundingBox &box)
{
    const Vector3Dd origin = ray.getOrigin();
    double invDirX, invDirY, invDirZ;
    bool degenerateX, degenerateY, degenerateZ;
    ray.getAabbSlabReciprocals(
        &invDirX, &invDirY, &invDirZ,
        &degenerateX, &degenerateY, &degenerateZ);
    double tMin = 0.0;
    double tMax = 1e30;

    auto updateAxis = [&](double originCoord, double invDir, bool degenerate,
                          double minCoord, double maxCoord) -> bool {
        if (degenerate) {
            return originCoord >= minCoord && originCoord <= maxCoord;
        }
        double nearT = (minCoord - originCoord) * invDir;
        double farT = (maxCoord - originCoord) * invDir;
        if (nearT > farT) {
            const double tmp = nearT;
            nearT = farT;
            farT = tmp;
        }
        tMin = nearT > tMin ? nearT : tMin;
        tMax = farT < tMax ? farT : tMax;
        return tMin <= tMax;
    };

    return
        updateAxis(origin.x(), invDirX, degenerateX, box.min.x(), box.max.x()) &&
        updateAxis(origin.y(), invDirY, degenerateY, box.min.y(), box.max.y()) &&
        updateAxis(origin.z(), invDirZ, degenerateZ, box.min.z(), box.max.z()) &&
        tMax >= 0.0;
}

bool
BakedTrace::pointInsideAabb(const Vector3Dd &point, const AxisAlignedBoundingBox &box, double tolerance)
{
    return
        point.x() >= box.min.x() - tolerance &&
        point.x() <= box.max.x() + tolerance &&
        point.y() >= box.min.y() - tolerance &&
        point.y() <= box.max.y() + tolerance &&
        point.z() >= box.min.z() - tolerance &&
        point.z() <= box.max.z() + tolerance;
}

bool
BakedTrace::finalizeSimpleBodyCandidate(
    const BakedScene &scene,
    const BakedScene::TraceableObject &baked,
    RayWithSegments *ray,
    IntersectionCandidate &candidate)
{
    if (baked.hasGeometryTransform) {
        candidate.getIntersection().point =
            baked.geometryToObject.transformPoint(candidate.getIntersection().point);
    }
    const Vector3Dd objectLocalPoint = candidate.getIntersection().point;

    if (baked.hasClippingShapes) {
        for (long int i = baked.clippingObjectIndices.size() - 1; i >= 0; i--) {
            if (BakedTrace::containmentTest(
                    scene,
                    baked.clippingObjectIndices[i],
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
BakedTrace::passesBoundingShapes(
    const BakedScene &scene,
    const BakedScene::TraceableObject &baked,
    RayWithSegments *objectRayPtr,
    RaySharedCache &cache)
{
    for (long int i = baked.boundingObjectIndices.size() - 1; i >= 0; i--) {
        IntersectionCandidate boundingHit;
        const Vector3Dd objectRayOrigin = objectRayPtr->getOrigin();
        const int boundingIndex = baked.boundingObjectIndices[i];
        if (!BakedTrace::traceFirstHit(scene, boundingIndex, objectRayPtr, boundingHit, cache) &&
            BakedTrace::containmentTest(
                scene,
                boundingIndex,
                objectRayOrigin,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
BakedTrace::traceSimpleBodyAllCrossings(
    const BakedScene &scene,
    const BakedScene::TraceableObject &baked,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache)
{
    if (baked.geometry == nullptr) {
        return false;
    }

    // Fast path: direct quadric simple body with no bounding/clipping shapes
    // and no CSG - ported verbatim from BakedSimpleBodyTracing::traceAllCrossings.
    if (!baked.hasBoundingShapes && !baked.hasClippingShapes &&
        baked.csgProgramIndex < 0 && baked.quadricGeometry != nullptr) {
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
        const bool sharesRaySpace =
            !baked.hasObjectTransform && !baked.hasGeometryTransform;

        double depth1;
        double depth2;
        if (!BakedCsgTrace::intersectBakedQuadric(
                *baked.quadricGeometry, ray, geomOrigin, geomDir,
                sharesRaySpace, cache, baked.quadricViewpointSlot,
                &depth1, &depth2)) {
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

    if (!passesBoundingShapes(scene, baked, objectRayPtr, cache)) {
        return false;
    }

    auto traceInGeometrySpace = [&](RayWithSegments *geometryRayPtr) -> bool {
        java::PriorityQueue<IntersectionCandidate> * const localDepthQueue =
            ray->getIntersectionQueuePool()->pop(128);
        const bool foundAny = baked.csgProgramIndex >= 0 ?
            (BakedCsgTrace::traceAllCrossings(
                 scene.csgPrograms[baked.csgProgramIndex],
                 scene.csgPrograms,
                 geometryRayPtr,
                 localDepthQueue,
                 cache,
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
            if (finalizeSimpleBodyCandidate(scene, baked, ray, candidate)) {
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

bool
BakedTrace::traceSimpleBodyFirstHit(
    const BakedScene &scene,
    const BakedScene::TraceableObject &baked,
    RayWithSegments *ray,
    IntersectionCandidate &out,
    RaySharedCache &cache)
{
    if (baked.geometry == nullptr) {
        return false;
    }

    const bool canUseGeometryFirstHit =
        baked.csgProgramIndex < 0 && !baked.hasBoundingShapes && !baked.hasClippingShapes;

    auto traceInObjectSpace = [&](RayWithSegments *objectRayPtr) -> bool {

    if (!passesBoundingShapes(scene, baked, objectRayPtr, cache)) {
        return false;
    }

    auto traceInGeometrySpace = [&](RayWithSegments *geometryRayPtr) -> bool {
    if (canUseGeometryFirstHit) {
        IntersectionCandidate candidate;
        if (baked.geometry->doIntersectionFirstHitNoQueue(
                geometryRayPtr, candidate, baked.geometryMaterial)) {
            if (!finalizeSimpleBodyCandidate(scene, baked, ray, candidate)) {
                return false;
            }
            out = candidate;
            return true;
        }
    }

    if (baked.csgProgramIndex >= 0) {
        IntersectionCandidate candidate;
        if (!BakedCsgTrace::traceFirstHit(
                scene.csgPrograms[baked.csgProgramIndex],
                scene.csgPrograms,
                geometryRayPtr,
                candidate,
                cache,
                baked.geometryMaterial)) {
            return false;
        }
        if (!finalizeSimpleBodyCandidate(scene, baked, ray, candidate)) {
            return false;
        }
        out = candidate;
        return true;
    }

    java::PriorityQueue<IntersectionCandidate> * const depthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    const bool hasHit = baked.geometry->doIntersectionForAllRayCrossings(
        geometryRayPtr, depthQueue, baked.geometryMaterial) && depthQueue->size() > 0;
    if (!hasHit) {
        ray->getIntersectionQueuePool()->push(depthQueue);
        return false;
    }

    bool found = false;
    if (!baked.hasClippingShapes) {
        IntersectionCandidate nearest = depthQueue->peek();
        if (finalizeSimpleBodyCandidate(scene, baked, ray, nearest)) {
            out = nearest;
            found = true;
        }
    } else {
        while (depthQueue->size() > 0) {
            IntersectionCandidate candidate = depthQueue->poll();
            if (finalizeSimpleBodyCandidate(scene, baked, ray, candidate)) {
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
BakedTrace::simpleBodyContainmentTest(
    const BakedScene &scene,
    const BakedScene::TraceableObject &baked,
    const Vector3Dd &point,
    double distanceTolerance)
{
    Vector3Dd localPoint = point;
    if (baked.hasObjectTransform) {
        localPoint = baked.worldToObject.transformPoint(point);
    }

    for (long int i = baked.boundingObjectIndices.size() - 1; i >= 0; i--) {
        if (BakedTrace::containmentTest(
                scene, baked.boundingObjectIndices[i], localPoint, distanceTolerance) ==
            Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    for (long int i = baked.clippingObjectIndices.size() - 1; i >= 0; i--) {
        if (BakedTrace::containmentTest(
                scene, baked.clippingObjectIndices[i], localPoint, distanceTolerance) ==
            Geometry::OUTSIDE) {
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
    const int containment = baked.csgProgramIndex >= 0 ?
        BakedCsgTrace::containmentTest(
            scene.csgPrograms[baked.csgProgramIndex],
            scene.csgPrograms,
            geometryPoint,
            distanceTolerance) :
        baked.geometry->doContainmentTest(geometryPoint, distanceTolerance);
    if (containment != Geometry::OUTSIDE) {
        return Geometry::INSIDE;
    }
    return Geometry::OUTSIDE;
}

bool
BakedTrace::traceCompositeAllCrossingsInCompositeSpace(
    const BakedScene &scene,
    const BakedScene::CompositeRecord &composite,
    RayWithSegments *ray,
    RayWithSegments *compositeRayPtr,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache)
{
    for (long int i = composite.boundingObjectIndices.size() - 1; i >= 0; i--) {
        IntersectionCandidate boundingHit;
        const Vector3Dd compositeRayOrigin = compositeRayPtr->getOrigin();
        const int boundingIndex = composite.boundingObjectIndices[i];
        if (!BakedTrace::traceFirstHit(scene, boundingIndex, compositeRayPtr, boundingHit, cache) &&
            BakedTrace::containmentTest(
                scene,
                boundingIndex,
                compositeRayOrigin,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }

    java::PriorityQueue<IntersectionCandidate> * const localDepthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    bool accepted = false;
    // Preserve Composite::doIntersectionForAllRayCrossings child traversal
    // order exactly; changing equal-depth ordering changes some legacy scenes.
    const java::ArrayList<int> &children = composite.childObjectIndices;
    for (long int i = children.size() - 1; i >= 0; i--) {
        const int childIndex = children[i];
        const BakedScene::TraceableObject &child = scene.traceableObjects[childIndex];
        if (!composite.hasObjectTransform &&
            child.bounded &&
            !rayIntersectsAabbForward(*compositeRayPtr, child.worldBounds)) {
            continue;
        }
        BakedTrace::traceAllCrossings(scene, childIndex, compositeRayPtr, localDepthQueue, cache);
    }

    for (IntersectionCandidate &candidate : *localDepthQueue) {
        candidate.getAttributes().pushDetailOwner(
            candidate.getAttributes().getHitBody());
        candidate.getAttributes().setHitBody(composite.object);
        const Vector3Dd compositeLocalPoint = candidate.getIntersection().point;
        if (composite.hasObjectTransform) {
            candidate.getIntersection().point =
                composite.objectToWorld.transformPoint(candidate.getIntersection().point);
        }

        bool intersectionFound = true;
        for (long int i = composite.clippingObjectIndices.size() - 1; i >= 0; i--) {
            if (BakedTrace::containmentTest(
                    scene,
                    composite.clippingObjectIndices[i],
                    compositeLocalPoint,
                    Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
                intersectionFound = false;
                break;
            }
        }
        if (!intersectionFound) {
            continue;
        }

        candidate.getIntersection().t =
            candidate.getIntersection().point.subtract(ray->getOrigin()).length();
        depthQueue->offer(candidate);
        accepted = true;
    }

    localDepthQueue->clear();
    ray->getIntersectionQueuePool()->push(localDepthQueue);
    return accepted;
}

bool
BakedTrace::traceCompositeAllCrossings(
    const BakedScene &scene,
    const BakedScene::CompositeRecord &composite,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache)
{
    if (composite.object == nullptr) {
        return false;
    }

    RayWithSegments *compositeRayPtr = ray;
    if (composite.hasObjectTransform) {
        RayWithSegments compositeRay(
            RayWithSegments::LocalIntersectionClone{}, *ray);
        compositeRay.setOrigin(composite.worldToObject.transformPoint(ray->getOrigin()));
        compositeRay.setDirection(composite.worldToObject.transformDirection(ray->getDirection()));
        compositeRay.setQuadricConstantsCached(false);
        compositeRayPtr = &compositeRay;

        return traceCompositeAllCrossingsInCompositeSpace(
            scene, composite, ray, compositeRayPtr, depthQueue, cache);
    }

    return traceCompositeAllCrossingsInCompositeSpace(
        scene, composite, ray, compositeRayPtr, depthQueue, cache);
}

bool
BakedTrace::traceCompositeFirstHit(
    const BakedScene &scene,
    const BakedScene::CompositeRecord &composite,
    RayWithSegments *ray,
    IntersectionCandidate &out,
    RaySharedCache &cache)
{
    java::PriorityQueue<IntersectionCandidate> * const depthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    bool hit = false;
    if (traceCompositeAllCrossings(scene, composite, ray, depthQueue, cache) &&
        depthQueue->size() > 0) {
        out = depthQueue->peek();
        hit = true;
    }
    ray->getIntersectionQueuePool()->push(depthQueue);
    return hit;
}

int
BakedTrace::compositeContainmentTest(
    const BakedScene &scene,
    const BakedScene::CompositeRecord &composite,
    const Vector3Dd &point,
    double distanceTolerance)
{
    Vector3Dd localPoint = point;
    if (composite.hasObjectTransform) {
        localPoint = composite.worldToObject.transformPoint(point);
    }

    for (long int i = composite.boundingObjectIndices.size() - 1; i >= 0; i--) {
        if (BakedTrace::containmentTest(
                scene,
                composite.boundingObjectIndices[i],
                localPoint,
                distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    for (long int i = composite.clippingObjectIndices.size() - 1; i >= 0; i--) {
        if (BakedTrace::containmentTest(
                scene,
                composite.clippingObjectIndices[i],
                localPoint,
                distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    for (long int i = composite.childObjectIndices.size() - 1; i >= 0; i--) {
        const int childIndex = composite.childObjectIndices[i];
        const BakedScene::TraceableObject &child = scene.traceableObjects[childIndex];
        if (child.bounded && !pointInsideAabb(localPoint, child.worldBounds, distanceTolerance)) {
            continue;
        }
        if (BakedTrace::containmentTest(scene, childIndex, localPoint, distanceTolerance) !=
            Geometry::OUTSIDE) {
            return Geometry::INSIDE;
        }
    }

    return Geometry::OUTSIDE;
}

bool
BakedTrace::traceFirstHit(
    const BakedScene &scene,
    int objectIndex,
    RayWithSegments *ray,
    IntersectionCandidate &out,
    RaySharedCache &cache)
{
    const BakedScene::TraceableObject &baked = scene.traceableObjects[objectIndex];
    switch (baked.kind) {
    case BakedScene::TraceKind::Empty:
        return false;
    case BakedScene::TraceKind::Composite:
        return traceCompositeFirstHit(
            scene, scene.composites[baked.compositeIndex], ray, out, cache);
    case BakedScene::TraceKind::Csg:
    case BakedScene::TraceKind::DirectPrimitive:
    case BakedScene::TraceKind::BoundedGeneric:
    case BakedScene::TraceKind::GenericFallback:
        return traceSimpleBodyFirstHit(scene, baked, ray, out, cache);
    }
    return false;
}

bool
BakedTrace::traceAllCrossings(
    const BakedScene &scene,
    int objectIndex,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache)
{
    const BakedScene::TraceableObject &baked = scene.traceableObjects[objectIndex];
    switch (baked.kind) {
    case BakedScene::TraceKind::Empty:
        return false;
    case BakedScene::TraceKind::Composite:
        return traceCompositeAllCrossings(
            scene, scene.composites[baked.compositeIndex], ray, depthQueue, cache);
    case BakedScene::TraceKind::Csg:
    case BakedScene::TraceKind::DirectPrimitive:
    case BakedScene::TraceKind::BoundedGeneric:
    case BakedScene::TraceKind::GenericFallback:
        return traceSimpleBodyAllCrossings(scene, baked, ray, depthQueue, cache);
    }
    return false;
}

int
BakedTrace::containmentTest(
    const BakedScene &scene,
    int objectIndex,
    const Vector3Dd &point,
    double distanceTolerance)
{
    const BakedScene::TraceableObject &baked = scene.traceableObjects[objectIndex];
    switch (baked.kind) {
    case BakedScene::TraceKind::Empty:
        return Geometry::OUTSIDE;
    case BakedScene::TraceKind::Composite:
        return compositeContainmentTest(
            scene, scene.composites[baked.compositeIndex], point, distanceTolerance);
    case BakedScene::TraceKind::Csg:
    case BakedScene::TraceKind::DirectPrimitive:
    case BakedScene::TraceKind::BoundedGeneric:
    case BakedScene::TraceKind::GenericFallback:
        return simpleBodyContainmentTest(scene, baked, point, distanceTolerance);
    }
    return Geometry::OUTSIDE;
}
