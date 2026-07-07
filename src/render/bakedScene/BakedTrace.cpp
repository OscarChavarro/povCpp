
#include "environment/scene/Composite.h"
#include "render/bakedScene/BakedTrace.h"
#include "render/bakedScene/CsgContainmentTest.h"
#include "render/bakedScene/CsgFirstHitTrace.h"
#include "render/bakedScene/CsgOperandTrace.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"

bool
BakedTrace::finalizeSimpleBodyCandidate(
    const BakedScene &scene,
    const TraceableObject *baked,
    RayWithTracingState *ray,
    IntersectionCandidate &candidate)
{
    if (baked->getHasGeometryTransform()) {
        candidate.getIntersection().point =
            baked->getGeometryToObject().transformPoint(candidate.getIntersection().point);
    }

    // Clipping shapes live in the object's outer frame (they only ever receive
    // object/composite-level transform deltas, never ones internal to a CSG
    // block), so containment must be tested after the object transform.
    Vector3Dd clipTestPoint = candidate.getIntersection().point;
    if (baked->getHasObjectTransform()) {
        clipTestPoint = baked->getObjectToWorld().transformPoint(clipTestPoint);
    }

    if (baked->getHasClippingShapes()) {
        for (long int i = baked->getClippingObjectIndices().size() - 1; i >= 0; i--) {
            if (BakedTrace::containmentTest(
                    scene,
                    baked->getClippingObjectIndices()[i],
                    clipTestPoint,
                    GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
                return false;
            }
        }
    }

    candidate.getAttributes().setObjectTexture(baked->getObjectTexture());
    candidate.getAttributes().setObjectColor(baked->getObjectColor());
    candidate.getAttributes().setNoShadowFlag(baked->getNoShadowFlag());
    candidate.getAttributes().setHitBody(baked->getObject());
    if (baked->getHasObjectTransform()) {
        candidate.getIntersection().point =
            baked->getObjectToWorld().transformPoint(candidate.getIntersection().point);
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
    const TraceableObject *baked,
    RayWithTracingState *objectRayPtr,
    RaySharedCache &cache)
{
    for (long int i = baked->getBoundingObjectIndices().size() - 1; i >= 0; i--) {
        IntersectionCandidate boundingHit;
        const Vector3Dd objectRayOrigin = objectRayPtr->getOrigin();
        const int boundingIndex = baked->getBoundingObjectIndices()[i];
        if (!BakedTrace::traceFirstHit(scene, boundingIndex, objectRayPtr, boundingHit, cache) &&
            BakedTrace::containmentTest(
                scene,
                boundingIndex,
                objectRayOrigin,
                GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
BakedTrace::traceSimpleBodyAllCrossings(
    const BakedScene &scene,
    const TraceableObject *baked,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache)
{
    if (baked->getGeometry() == nullptr) {
        return false;
    }

    // Fast path: direct quadric simple body with no bounding/clipping shapes
    // and no CSG - ported verbatim from BakedSimpleBodyTracing::traceAllCrossings.
    if (!baked->getHasBoundingShapes() && !baked->getHasClippingShapes() &&
        baked->getCsgProgramIndex() < 0 && baked->getQuadricGeometry() != nullptr) {
        const Vector3Dd worldOrigin = ray->getOrigin();
        const Vector3Dd worldDir = ray->getDirection();
        const Vector3Dd localOrigin = baked->getHasObjectTransform() ?
            baked->getWorldToObject().transformPoint(worldOrigin) : worldOrigin;
        const Vector3Dd localDir = baked->getHasObjectTransform() ?
            baked->getWorldToObject().transformDirection(worldDir) : worldDir;
        const Vector3Dd geomOrigin = baked->getHasGeometryTransform() ?
            baked->getObjectToGeometry().transformPoint(localOrigin) : localOrigin;
        const Vector3Dd geomDir = baked->getHasGeometryTransform() ?
            baked->getObjectToGeometry().transformDirection(localDir) : localDir;
        const bool sharesRaySpace =
            !baked->getHasObjectTransform() && !baked->getHasGeometryTransform();

        double depth1;
        double depth2;
        if (!BakedQuadricIntersector::intersectBakedQuadric(
                *baked->getQuadricGeometry(), ray, geomOrigin, geomDir,
                sharesRaySpace, cache, baked->getQuadricViewpointSlot(),
                &depth1, &depth2)) {
            return false;
        }

        const auto offerQuadricCandidate = [&](double depth) {
            IntersectionCandidate candidate;
            candidate.getIntersection().point = geomOrigin.add(geomDir.multiply(depth));
            candidate.getAttributes().setHitGeometry(baked->getQuadricGeometry());
            candidate.getAttributes().setMaterial(baked->getGeometryMaterial());
            if (baked->getHasGeometryTransform()) {
                candidate.getIntersection().point =
                    baked->getGeometryToObject().transformPoint(candidate.getIntersection().point);
            }
            candidate.getAttributes().setObjectTexture(baked->getObjectTexture());
            candidate.getAttributes().setObjectColor(baked->getObjectColor());
            candidate.getAttributes().setNoShadowFlag(baked->getNoShadowFlag());
            candidate.getAttributes().setHitBody(baked->getObject());
            if (baked->getHasObjectTransform()) {
                candidate.getIntersection().point =
                    baked->getObjectToWorld().transformPoint(candidate.getIntersection().point);
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

    auto traceInObjectSpace = [&](RayWithTracingState *objectRayPtr) -> bool {

    if (!passesBoundingShapes(scene, baked, objectRayPtr, cache)) {
        return false;
    }

    auto traceInGeometrySpace = [&](RayWithTracingState *geometryRayPtr) -> bool {
        java::PriorityQueue<IntersectionCandidate> * const localDepthQueue =
            ray->getIntersectionQueuePool()->pop(128);
        const bool foundAny = baked->getCsgProgramIndex() >= 0 ?
            (CsgOperandTrace::traceAllCrossings(
                 scene.csgPrograms[baked->getCsgProgramIndex()],
                 scene.csgPrograms,
                 geometryRayPtr,
                 localDepthQueue,
                 cache,
                 baked->getGeometryMaterial()) &&
             localDepthQueue->size() > 0) :
            (baked->getGeometry()->doIntersectionForAllRayCrossings(
                 geometryRayPtr, localDepthQueue, baked->getGeometryMaterial()) &&
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

    if (baked->getHasGeometryTransform()) {
        RayWithTracingState geometryRay = RayWithTracingState::localIntersectionClone(*objectRayPtr);
        geometryRay.setOrigin(
            baked->getObjectToGeometry().transformPoint(objectRayPtr->getOrigin()));
        geometryRay.setDirection(
            baked->getObjectToGeometry().transformDirection(objectRayPtr->getDirection()));
        geometryRay.setQuadricConstantsCached(false);
        return traceInGeometrySpace(&geometryRay);
    }
    return traceInGeometrySpace(objectRayPtr);
    };

    if (baked->getHasObjectTransform()) {
        RayWithTracingState objectRay = RayWithTracingState::localIntersectionClone(*ray);
        objectRay.setOrigin(baked->getWorldToObject().transformPoint(ray->getOrigin()));
        objectRay.setDirection(baked->getWorldToObject().transformDirection(ray->getDirection()));
        objectRay.setQuadricConstantsCached(false);
        return traceInObjectSpace(&objectRay);
    }
    return traceInObjectSpace(ray);
}

bool
BakedTrace::traceSimpleBodyFirstHit(
    const BakedScene &scene,
    const TraceableObject *baked,
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    RaySharedCache &cache)
{
    if (baked->getGeometry() == nullptr) {
        return false;
    }

    const bool canUseGeometryFirstHit =
        baked->getCsgProgramIndex() < 0 && !baked->getHasBoundingShapes() && !baked->getHasClippingShapes();

    auto traceInObjectSpace = [&](RayWithTracingState *objectRayPtr) -> bool {

    if (!passesBoundingShapes(scene, baked, objectRayPtr, cache)) {
        return false;
    }

    auto traceInGeometrySpace = [&](RayWithTracingState *geometryRayPtr) -> bool {
    if (canUseGeometryFirstHit) {
        IntersectionCandidate candidate;
        if (baked->getGeometry()->doIntersectionFirstHit(
                geometryRayPtr, candidate, baked->getGeometryMaterial())) {
            if (!finalizeSimpleBodyCandidate(scene, baked, ray, candidate)) {
                return false;
            }
            out = candidate;
            return true;
        }
    }

    if (baked->getCsgProgramIndex() >= 0) {
        IntersectionCandidate candidate;
        if (!CsgFirstHitTrace::traceFirstHit(
                scene.csgPrograms[baked->getCsgProgramIndex()],
                scene.csgPrograms,
                geometryRayPtr,
                candidate,
                cache,
                baked->getGeometryMaterial())) {
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
    const bool hasHit = baked->getGeometry()->doIntersectionForAllRayCrossings(
        geometryRayPtr, depthQueue, baked->getGeometryMaterial()) && depthQueue->size() > 0;
    if (!hasHit) {
        ray->getIntersectionQueuePool()->push(depthQueue);
        return false;
    }

    bool found = false;
    if (!baked->getHasClippingShapes()) {
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

    if (baked->getHasGeometryTransform()) {
        RayWithTracingState geometryRay = RayWithTracingState::localIntersectionClone(*objectRayPtr);
        geometryRay.setOrigin(
            baked->getObjectToGeometry().transformPoint(objectRayPtr->getOrigin()));
        geometryRay.setDirection(
            baked->getObjectToGeometry().transformDirection(objectRayPtr->getDirection()));
        geometryRay.setQuadricConstantsCached(false);
        return traceInGeometrySpace(&geometryRay);
    }
    return traceInGeometrySpace(objectRayPtr);
    };

    if (baked->getHasObjectTransform()) {
        RayWithTracingState objectRay = RayWithTracingState::localIntersectionClone(*ray);
        objectRay.setOrigin(baked->getWorldToObject().transformPoint(ray->getOrigin()));
        objectRay.setDirection(baked->getWorldToObject().transformDirection(ray->getDirection()));
        objectRay.setQuadricConstantsCached(false);
        return traceInObjectSpace(&objectRay);
    }
    return traceInObjectSpace(ray);
}

int
BakedTrace::simpleBodyContainmentTest(
    const BakedScene &scene,
    const TraceableObject *baked,
    const Vector3Dd &point,
    double distanceTolerance)
{
    Vector3Dd localPoint = point;
    if (baked->getHasObjectTransform()) {
        localPoint = baked->getWorldToObject().transformPoint(point);
    }

    // Bounding/clipping shapes live in this object's outer frame (the same
    // frame `point` arrives in), not in its own local/geometry frame.
    for (long int i = baked->getBoundingObjectIndices().size() - 1; i >= 0; i--) {
        if (BakedTrace::containmentTest(
                scene, baked->getBoundingObjectIndices()[i], point, distanceTolerance) ==
            Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    for (long int i = baked->getClippingObjectIndices().size() - 1; i >= 0; i--) {
        if (BakedTrace::containmentTest(
                scene, baked->getClippingObjectIndices()[i], point, distanceTolerance) ==
            Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    if (baked->getGeometry() == nullptr) {
        return Geometry::OUTSIDE;
    }

    Vector3Dd geometryPoint = localPoint;
    if (baked->getHasGeometryTransform()) {
        geometryPoint = baked->getObjectToGeometry().transformPoint(localPoint);
    }
    const int containment = baked->getCsgProgramIndex() >= 0 ?
        CsgContainmentTest::containmentTest(
            scene.csgPrograms[baked->getCsgProgramIndex()],
            scene.csgPrograms,
            geometryPoint,
            distanceTolerance) :
        baked->getGeometry()->doContainmentTest(geometryPoint, distanceTolerance);
    if (containment != Geometry::OUTSIDE) {
        return Geometry::INSIDE;
    }
    return Geometry::OUTSIDE;
}

bool
BakedTrace::traceCompositeAllCrossingsInCompositeSpace(
    const BakedScene &scene,
    const CompositeRecord *composite,
    RayWithTracingState *ray,
    RayWithTracingState *compositeRayPtr,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache)
{
    for (long int i = composite->getBoundingObjectIndices().size() - 1; i >= 0; i--) {
        IntersectionCandidate boundingHit;
        const Vector3Dd compositeRayOrigin = compositeRayPtr->getOrigin();
        const int boundingIndex = composite->getBoundingObjectIndices()[i];
        if (!BakedTrace::traceFirstHit(scene, boundingIndex, compositeRayPtr, boundingHit, cache) &&
            BakedTrace::containmentTest(
                scene,
                boundingIndex,
                compositeRayOrigin,
                GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }

    java::PriorityQueue<IntersectionCandidate> * const localDepthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    bool accepted = false;
    // Preserve Composite::doIntersectionForAllRayCrossings child traversal
    // order exactly; changing equal-depth ordering changes some legacy scenes.
    const java::ArrayList<int> &children = composite->getChildObjectIndices();
    for (long int i = children.size() - 1; i >= 0; i--) {
        const int childIndex = children[i];
        const TraceableObject *child = scene.traceableObjects[childIndex];
        if (!composite->getHasObjectTransform() &&
            child->getBounded() &&
            !child->getWorldBounds().intersectsRayForward(*compositeRayPtr)) {
            continue;
        }
        BakedTrace::traceAllCrossings(scene, childIndex, compositeRayPtr, localDepthQueue, cache);
    }

    for (IntersectionCandidate &candidate : *localDepthQueue) {
        candidate.getAttributes().pushDetailOwner(
            candidate.getAttributes().getHitBody());
        candidate.getAttributes().setHitBody(composite->getObject());
        if (composite->getHasObjectTransform()) {
            candidate.getIntersection().point =
                composite->getObjectToWorld().transformPoint(candidate.getIntersection().point);
        }

        // The composite's own clipping shapes live in its outer frame, so
        // containment must be tested after the composite's object transform.
        bool intersectionFound = true;
        for (long int i = composite->getClippingObjectIndices().size() - 1; i >= 0; i--) {
            if (BakedTrace::containmentTest(
                    scene,
                    composite->getClippingObjectIndices()[i],
                    candidate.getIntersection().point,
                    GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
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
    const CompositeRecord *composite,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache)
{
    if (composite->getObject() == nullptr) {
        return false;
    }

    RayWithTracingState *compositeRayPtr = ray;
    if (composite->getHasObjectTransform()) {
        RayWithTracingState compositeRay = RayWithTracingState::localIntersectionClone(*ray);
        compositeRay.setOrigin(composite->getWorldToObject().transformPoint(ray->getOrigin()));
        compositeRay.setDirection(composite->getWorldToObject().transformDirection(ray->getDirection()));
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
    const CompositeRecord *composite,
    RayWithTracingState *ray,
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
    const CompositeRecord *composite,
    const Vector3Dd &point,
    double distanceTolerance)
{
    Vector3Dd localPoint = point;
    if (composite->getHasObjectTransform()) {
        localPoint = composite->getWorldToObject().transformPoint(point);
    }

    // The composite's own bounding/clipping shapes live in its outer frame
    // (the frame `point` arrives in), not in the composite-local frame used
    // by its children below.
    for (long int i = composite->getBoundingObjectIndices().size() - 1; i >= 0; i--) {
        if (BakedTrace::containmentTest(
                scene,
                composite->getBoundingObjectIndices()[i],
                point,
                distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    for (long int i = composite->getClippingObjectIndices().size() - 1; i >= 0; i--) {
        if (BakedTrace::containmentTest(
                scene,
                composite->getClippingObjectIndices()[i],
                point,
                distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    for (long int i = composite->getChildObjectIndices().size() - 1; i >= 0; i--) {
        const int childIndex = composite->getChildObjectIndices()[i];
        const TraceableObject *child = scene.traceableObjects[childIndex];
        if (child->getBounded() &&
            !child->getWorldBounds().containsPointWithTolerance(localPoint, distanceTolerance)) {
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
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    RaySharedCache &cache)
{
    const TraceableObject *baked = scene.traceableObjects[objectIndex];
    switch (baked->getKind()) {
    case BakedScene::TraceKind::Empty:
        return false;
    case BakedScene::TraceKind::Composite:
        return traceCompositeFirstHit(
            scene, scene.composites[baked->getCompositeIndex()], ray, out, cache);
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
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache)
{
    const TraceableObject *baked = scene.traceableObjects[objectIndex];
    switch (baked->getKind()) {
    case BakedScene::TraceKind::Empty:
        return false;
    case BakedScene::TraceKind::Composite:
        return traceCompositeAllCrossings(
            scene, scene.composites[baked->getCompositeIndex()], ray, depthQueue, cache);
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
    const TraceableObject *baked = scene.traceableObjects[objectIndex];
    switch (baked->getKind()) {
    case BakedScene::TraceKind::Empty:
        return Geometry::OUTSIDE;
    case BakedScene::TraceKind::Composite:
        return compositeContainmentTest(
            scene, scene.composites[baked->getCompositeIndex()], point, distanceTolerance);
    case BakedScene::TraceKind::Csg:
    case BakedScene::TraceKind::DirectPrimitive:
    case BakedScene::TraceKind::BoundedGeneric:
    case BakedScene::TraceKind::GenericFallback:
        return simpleBodyContainmentTest(scene, baked, point, distanceTolerance);
    }
    return Geometry::OUTSIDE;
}
