#include "environment/geometry/trace/BoxTrace.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "java/util/PriorityQueue.txx"

bool
BoxTrace::intersect(const RayWithTracingState *ray, const Box &box, double *depth1,
    double *depth2)
{
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayBoxTests();
    double tmin = 0.0;
    double tmax = HUGE_VAL;
    bool faceOverlap = false;
    if (Box::intersectBounds(ray->getOrigin(), ray->getDirection(),
            box.getRawBounds()[0], box.getRawBounds()[1], GeometryConfig::INTERSECTION_EPSILON,
            false, tmin, tmax, faceOverlap) != RayFirstHitStatus::Hit) {
        return false;
    }

    *depth1 = tmin;
    *depth2 = tmax;

    if ((*depth1 < GeometryConfig::SMALL_TOLERANCE) || (*depth1 > GeometryConfig::MAX_DISTANCE)) {
        if ((*depth2 < GeometryConfig::SMALL_TOLERANCE) || (*depth2 > GeometryConfig::MAX_DISTANCE)) {
            return false;
        }
        *depth1 = *depth2;

    } else if ((*depth2 < GeometryConfig::SMALL_TOLERANCE) || (*depth2 > GeometryConfig::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.incrementRayBoxTestsSucceeded();
    return true;
}

int
BoxTrace::emitCrossings(Box &box, RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride, const GeometryIntersectionEmissionContext *context)
{
    double depth1;
    double depth2;

    if (context == nullptr) {
        Vector3Dd intersectionPoint;
        IntersectionCandidate localElement;
        bool intersectionFound = false;
        if (intersect(ray, box, &depth1, &depth2)) {
            localElement.getIntersection().t = depth1;
            intersectionPoint = ray->getDirection().multiply(depth1);
            intersectionPoint = intersectionPoint.add(ray->getOrigin());
            localElement.getIntersection().point = intersectionPoint;
            localElement.getAttributes().setHitSurface(&box, SurfaceDetailFamily::Box);
            localElement.getAttributes().setMaterial(materialOverride);
            depthQueue->offer(localElement);
            intersectionFound = true;

            if (depth2 != depth1) {
                localElement.getIntersection().t = depth2;
                intersectionPoint = ray->getDirection().multiply(depth2);
                intersectionPoint = intersectionPoint.add(ray->getOrigin());
                localElement.getIntersection().point = intersectionPoint;
                localElement.getAttributes().setHitSurface(&box, SurfaceDetailFamily::Box);
                localElement.getAttributes().setMaterial(materialOverride);
                depthQueue->offer(localElement);
                intersectionFound = true;
            }
        }
        return intersectionFound;
    }

    if (!intersect(ray, box, &depth1, &depth2)) {
        return false;
    }

    IntersectionCandidate localElement;
    localElement.getAttributes().setHitSurface(&box, SurfaceDetailFamily::Box);
    localElement.getAttributes().setMaterial(context->getMaterialOverride());
    localElement.getAttributes().pushDetailOwner(context->getDetailOwner());
    localElement.getAttributes().setMaterialUsesObjectLocalPoint(
        context->getMaterialUsesObjectLocalPoint());

    localElement.getIntersection().t = depth1;
    localElement.getIntersection().point =
        ray->getDirection().multiply(depth1).add(ray->getOrigin());
    depthQueue->offer(localElement);

    if (depth2 != depth1) {
        localElement.getIntersection().t = depth2;
        localElement.getIntersection().point =
            ray->getDirection().multiply(depth2).add(ray->getOrigin());
        depthQueue->offer(localElement);
    }

    return true;
}

bool
BoxTrace::firstHit(Box &box, RayWithTracingState *ray, IntersectionCandidate &out,
    Material *materialOverride)
{
    double depth1;
    double depth2;
    if (!intersect(ray, box, &depth1, &depth2)) {
        return false;
    }

    const double nearestDepth = depth1 < depth2 ? depth1 : depth2;
    out.getIntersection().t = nearestDepth;
    out.getIntersection().point =
        ray->getDirection().multiply(nearestDepth).add(ray->getOrigin());
    out.getAttributes().setHitSurface(&box, SurfaceDetailFamily::Box);
    out.getAttributes().setMaterial(materialOverride);
    return true;
}

int
BoxTrace::containment(const Box &box, const SurfaceTraceState &state, const Vector3Dd &point,
    double distanceTolerance)
{
    (void)distanceTolerance;
    const bool inside = Box::containsBounds(point, box.getRawBounds()[0], box.getRawBounds()[1]);
    return inside != state.isInverted() ?
        static_cast<int>(Containment::INSIDE) : static_cast<int>(Containment::OUTSIDE);
}
