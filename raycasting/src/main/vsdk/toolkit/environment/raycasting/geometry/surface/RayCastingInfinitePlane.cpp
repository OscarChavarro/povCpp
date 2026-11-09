#include "vsdk/toolkit/environment/raycasting/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/IntersectionCandidate.h"
#include "vsdk/toolkit/environment/raycasting/geometry/surface/RayCastingInfinitePlane.h"
#include "java/util/PriorityQueue.txx"

RayCastingInfinitePlane::RayCastingInfinitePlane() :
    RayCastingInfinitePlane(Vector3Dd(0.0, 1.0, 0.0), 0.0)
{
}

RayCastingInfinitePlane::RayCastingInfinitePlane(const Vector3Dd &normalVector, double distance) :
    RayCastingInfinitePlane(normalVector, distance, 0.0, false)
{
}

RayCastingInfinitePlane::RayCastingInfinitePlane(const Vector3Dd &normalVector, double distance,
    double vpNormDotOrigin, bool vpCached) :
    normalVector(normalVector),
    distance(distance),
    vpNormDotOrigin(vpNormDotOrigin),
    vpCached(vpCached)
{
}

int
RayCastingInfinitePlane::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    RayCastingInfinitePlane * const shape = this;
    double depth;
    Vector3Dd intersectionPoint;

    if (RayCastingInfinitePlane::intersectPlane(ray, shape, &depth)) {
        if (depth > GeometryConfig::SMALL_TOLERANCE) {
            // Construct the candidate only on a real hit: the default ctor zeroes
            // ~168 bytes, wasted on the ~36% of plane tests that miss.
            IntersectionCandidate localElement;
            localElement.getIntersection().t = depth;
            intersectionPoint = ray->getDirection().multiply(depth);
            intersectionPoint = intersectionPoint.add(ray->getOrigin());
            localElement.getIntersection().point = intersectionPoint;
            localElement.getAttributes().setHitGeometry(shape);
            localElement.getAttributes().setMaterial(materialOverride);
            depthQueue->offer(localElement);
            return (true);
        }
    }

    return (false);
}

bool
RayCastingInfinitePlane::doIntersectionFirstHit(
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    RayCastingInfinitePlane * const shape = this;
    double depth;
    if (!RayCastingInfinitePlane::intersectPlane(ray, shape, &depth) ||
        depth <= GeometryConfig::SMALL_TOLERANCE) {
        return false;
    }

    out.getIntersection().t = depth;
    out.getIntersection().point =
        ray->getDirection().multiply(depth).add(ray->getOrigin());
    out.getAttributes().setHitGeometry(shape);
    out.getAttributes().setMaterial(materialOverride);
    return true;
}

int
RayCastingInfinitePlane::intersectPlane(
    RayWithTracingState *ray, RayCastingInfinitePlane *plane, double *depth)
{
    double normalDotOrigin;
    double normalDotDirection;
    GeometryStatistics &stats = *ray->getGeometryStatistics();

    stats.incrementRayPlaneTests();
    if (ray->isPrimaryRayEnabled()) {
        if (!plane->vpCached) {
            plane->vpNormDotOrigin =
                plane->normalVector.dotProduct(ray->getOrigin());
            plane->vpNormDotOrigin += plane->distance;
            plane->vpNormDotOrigin *= -1.0;
            plane->vpCached = true;
        }

        normalDotDirection = plane->normalVector.dotProduct(ray->getDirection());
        if ((normalDotDirection < GeometryConfig::SMALL_TOLERANCE) &&
            (normalDotDirection > -GeometryConfig::SMALL_TOLERANCE)) {
            return (false);
        }

        *depth = plane->vpNormDotOrigin / normalDotDirection;
        if ((*depth >= GeometryConfig::SMALL_TOLERANCE) && (*depth <= GeometryConfig::MAX_DISTANCE)) {
            stats.incrementRayPlaneTestsSucceeded();
            return (true);
        }
        return (false);
    }
    normalDotOrigin = plane->normalVector.dotProduct(ray->getOrigin());
    normalDotOrigin += plane->distance;
    normalDotOrigin *= -1.0;

    normalDotDirection = plane->normalVector.dotProduct(ray->getDirection());
    if ((normalDotDirection < GeometryConfig::SMALL_TOLERANCE) &&
        (normalDotDirection > -GeometryConfig::SMALL_TOLERANCE)) {
        return (false);
    }

    *depth = normalDotOrigin / normalDotDirection;
    if ((*depth >= GeometryConfig::SMALL_TOLERANCE) && (*depth <= GeometryConfig::MAX_DISTANCE)) {
        stats.incrementRayPlaneTestsSucceeded();
        return (true);
    }
    return (false);
}

int
RayCastingInfinitePlane::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    const RayCastingInfinitePlane *plane = this;

    double temp = testPoint.dotProduct(plane->normalVector);
    return ((temp + plane->distance) <= distanceTolerance) ? INSIDE : OUTSIDE;
}

void
RayCastingInfinitePlane::computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    *result = normalVector;
}

void *
RayCastingInfinitePlane::copy()
{
    return new RayCastingInfinitePlane(*this);
}

void
RayCastingInfinitePlane::invertGeometry()
{
    RayCastingInfinitePlane * const plane = this;

    plane->normalVector = plane->normalVector.multiply(-1.0);
    plane->distance *= -1.0;
}
