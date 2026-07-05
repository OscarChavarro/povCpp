#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/Config.h"
#include "vsdk/toolkit/common/statistics/GeometryStatistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/surface/InfinitePlane.h"

InfinitePlane::InfinitePlane() :
    InfinitePlane(Vector3Dd(0.0, 1.0, 0.0), 0.0)
{
}

InfinitePlane::InfinitePlane(const Vector3Dd &normalVector, double distance) :
    InfinitePlane(normalVector, distance, 0.0, false)
{
}

InfinitePlane::InfinitePlane(const Vector3Dd &normalVector, double distance,
    double vpNormDotOrigin, bool vpCached) :
    normalVector(normalVector),
    distance(distance),
    vpNormDotOrigin(vpNormDotOrigin),
    vpCached(vpCached)
{
}

int
InfinitePlane::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    InfinitePlane * const shape = this;
    double depth;
    Vector3Dd intersectionPoint;

    if (InfinitePlane::intersectPlane(ray, shape, &depth)) {
        if (depth > Config::SMALL_TOLERANCE) {
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
InfinitePlane::doIntersectionFirstHit(
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    InfinitePlane * const shape = this;
    double depth;
    if (!InfinitePlane::intersectPlane(ray, shape, &depth) ||
        depth <= Config::SMALL_TOLERANCE) {
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
InfinitePlane::intersectPlane(
    RayWithTracingState *ray, InfinitePlane *plane, double *depth)
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
        if ((normalDotDirection < Config::SMALL_TOLERANCE) &&
            (normalDotDirection > -Config::SMALL_TOLERANCE)) {
            return (false);
        }

        *depth = plane->vpNormDotOrigin / normalDotDirection;
        if ((*depth >= Config::SMALL_TOLERANCE) && (*depth <= Config::MAX_DISTANCE)) {
            stats.incrementRayPlaneTestsSucceeded();
            return (true);
        }
        return (false);
    }
    normalDotOrigin = plane->normalVector.dotProduct(ray->getOrigin());
    normalDotOrigin += plane->distance;
    normalDotOrigin *= -1.0;

    normalDotDirection = plane->normalVector.dotProduct(ray->getDirection());
    if ((normalDotDirection < Config::SMALL_TOLERANCE) &&
        (normalDotDirection > -Config::SMALL_TOLERANCE)) {
        return (false);
    }

    *depth = normalDotOrigin / normalDotDirection;
    if ((*depth >= Config::SMALL_TOLERANCE) && (*depth <= Config::MAX_DISTANCE)) {
        stats.incrementRayPlaneTestsSucceeded();
        return (true);
    }
    return (false);
}

int
InfinitePlane::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    const InfinitePlane *plane = this;

    double temp = testPoint.dotProduct(plane->normalVector);
    return ((temp + plane->distance) <= distanceTolerance) ? INSIDE : OUTSIDE;
}

void
InfinitePlane::computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    *result = normalVector;
}

void *
InfinitePlane::copy()
{
    return new InfinitePlane(*this);
}

void
InfinitePlane::invertGeometry()
{
    InfinitePlane * const plane = this;

    plane->normalVector = plane->normalVector.multiply(-1.0);
    plane->distance *= -1.0;
}
