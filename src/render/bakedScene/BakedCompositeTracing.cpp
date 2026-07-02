#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "common/Config.h"
#include "environment/scene/Composite.h"
#include "render/bakedScene/BakedCompositeTracing.h"
#include "render/bakedScene/BakedTracingCommon.h"

bool
BakedCompositeTracing::rayIntersectsAabbForward(const RayWithSegments &ray, const AxisAlignedBox &box)
{
    const Vector3Dd origin = ray.getOrigin();
    const Vector3Dd direction = ray.getDirection();
    double tMin = 0.0;
    double tMax = 1e30;

    auto updateAxis = [&](double originCoord, double directionCoord,
                          double minCoord, double maxCoord) -> bool {
        if (directionCoord > -1e-12 && directionCoord < 1e-12) {
            return originCoord >= minCoord && originCoord <= maxCoord;
        }
        const double invDir = 1.0 / directionCoord;
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
        updateAxis(origin.x(), direction.x(), box.min.x(), box.max.x()) &&
        updateAxis(origin.y(), direction.y(), box.min.y(), box.max.y()) &&
        updateAxis(origin.z(), direction.z(), box.min.z(), box.max.z()) &&
        tMax >= 0.0;
}

bool
BakedCompositeTracing::pointInsideAabb(const Vector3Dd &point, const AxisAlignedBox &box, double tolerance)
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
BakedCompositeTracing::traceAllCrossings(
    const Scene::BakedComposite &bakedComposite,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    if (bakedComposite.object == nullptr) {
        return false;
    }

    RayWithSegments *compositeRayPtr = ray;
    if (bakedComposite.hasObjectTransform) {
        RayWithSegments compositeRay(
            RayWithSegments::LocalIntersectionClone{}, *ray);
        compositeRay.setOrigin(
            bakedComposite.worldToObject.transformPoint(ray->getOrigin()));
        compositeRay.setDirection(
            bakedComposite.worldToObject.transformDirection(ray->getDirection()));
        compositeRay.setQuadricConstantsCached(false);
        compositeRayPtr = &compositeRay;

        return traceAllCrossingsInCompositeSpace(
            bakedComposite,
            bakedSimpleBodies,
            bakedCsgs,
            bakedComposites,
            ray,
            compositeRayPtr,
            depthQueue);
    }

    return traceAllCrossingsInCompositeSpace(
        bakedComposite,
        bakedSimpleBodies,
        bakedCsgs,
        bakedComposites,
        ray,
        compositeRayPtr,
        depthQueue);
}

bool
BakedCompositeTracing::traceAllCrossingsInCompositeSpace(
    const Scene::BakedComposite &bakedComposite,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    RayWithSegments *ray,
    RayWithSegments *compositeRayPtr,
    java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    for (long int i = bakedComposite.boundingObjects.size() - 1; i >= 0; i--) {
        IntersectionCandidate boundingHit;
        const Vector3Dd compositeRayOrigin = compositeRayPtr->getOrigin();
        if (!BakedTracingCommon::traceObjectFirstHit(
                bakedComposite.boundingObjects[i],
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
                compositeRayPtr,
                boundingHit) &&
            BakedTracingCommon::containmentTest(
                bakedComposite.boundingObjects[i],
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
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
    const java::ArrayList<Scene::CompiledTracingObject> &children =
        bakedComposite.childObjects;
    for (long int i = children.size() - 1; i >= 0; i--) {
        if (!bakedComposite.hasObjectTransform &&
            children[i].bounded &&
            !rayIntersectsAabbForward(*compositeRayPtr, children[i].bounds)) {
            continue;
        }
        BakedTracingCommon::traceObjectAllCrossings(
            children[i],
            bakedSimpleBodies,
            bakedCsgs,
            bakedComposites,
            compositeRayPtr,
            localDepthQueue);
    }

    for (IntersectionCandidate &candidate : *localDepthQueue) {
        candidate.getAttributes().pushDetailOwner(
            candidate.getAttributes().getHitBody());
        candidate.getAttributes().setHitBody(bakedComposite.object);
        const Vector3Dd compositeLocalPoint = candidate.getIntersection().point;
        if (bakedComposite.hasObjectTransform) {
            candidate.getIntersection().point =
                bakedComposite.objectToWorld.transformPoint(
                    candidate.getIntersection().point);
        }

        bool intersectionFound = true;
        for (long int i = bakedComposite.clippingObjects.size() - 1; i >= 0; i--) {
            if (BakedTracingCommon::containmentTest(
                    bakedComposite.clippingObjects[i],
                    bakedSimpleBodies,
                    bakedCsgs,
                    bakedComposites,
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
BakedCompositeTracing::traceFirstHit(
    const Scene::BakedComposite &bakedComposite,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    RayWithSegments *ray,
    IntersectionCandidate &out)
{
    java::PriorityQueue<IntersectionCandidate> * const depthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    bool hit = false;
    if (traceAllCrossings(
            bakedComposite,
            bakedSimpleBodies,
            bakedCsgs,
            bakedComposites,
            ray,
            depthQueue) &&
        depthQueue->size() > 0) {
        out = depthQueue->peek();
        hit = true;
    }
    ray->getIntersectionQueuePool()->push(depthQueue);
    return hit;
}

int
BakedCompositeTracing::containmentTest(
    const Scene::BakedComposite &bakedComposite,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    const Vector3Dd &point,
    double distanceTolerance)
{
    Vector3Dd localPoint = point;
    if (bakedComposite.hasObjectTransform) {
        localPoint = bakedComposite.worldToObject.transformPoint(point);
    }

    for (long int i = bakedComposite.boundingObjects.size() - 1; i >= 0; i--) {
        if (BakedTracingCommon::containmentTest(
                bakedComposite.boundingObjects[i],
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
                localPoint,
                distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    for (long int i = bakedComposite.clippingObjects.size() - 1; i >= 0; i--) {
        if (BakedTracingCommon::containmentTest(
                bakedComposite.clippingObjects[i],
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
                localPoint,
                distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    for (long int i = bakedComposite.childObjects.size() - 1; i >= 0; i--) {
        const Scene::CompiledTracingObject &child = bakedComposite.childObjects[i];
        if (child.bounded &&
            !pointInsideAabb(localPoint, child.bounds, distanceTolerance)) {
            continue;
        }
        if (BakedTracingCommon::containmentTest(
                child,
                bakedSimpleBodies,
                bakedCsgs,
                bakedComposites,
                localPoint,
                distanceTolerance) != Geometry::OUTSIDE) {
            return Geometry::INSIDE;
        }
    }

    return Geometry::OUTSIDE;
}
