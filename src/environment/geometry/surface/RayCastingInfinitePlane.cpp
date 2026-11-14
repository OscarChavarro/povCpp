#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/surface/RayCastingInfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
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
    halfSpace(normalVector.x(), normalVector.y(), normalVector.z(), distance),
    vpNormDotOrigin(vpNormDotOrigin),
    vpCached(vpCached)
{
}

void RayCastingInfinitePlane::getBounds(GeometryBounds& result) const
{
    halfSpace.getBounds(result);
}

GeometryContainment RayCastingInfinitePlane::classifyPoint(const Vector3Dd& point,
    double tolerance) const
{
    return halfSpace.classifyPoint(point, tolerance);
}

RayCrossingSupport RayCastingInfinitePlane::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    return halfSpace.emitRayCrossings(ray, query, sink);
}

RayFirstHitStatus RayCastingInfinitePlane::queryFirstHit(const Ray& ray,
    const RayCrossingQuery& query, RayHit* hit, RayBoundary* boundary) const
{
    return halfSpace.queryFirstHit(ray, query, hit, boundary);
}

RaySolidQueryStatus RayCastingInfinitePlane::classifyRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    return halfSpace.classifyRay(ray, query, result);
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
                plane->getNormalVector().dotProduct(ray->getOrigin());
            plane->vpNormDotOrigin += plane->getDistance();
            plane->vpNormDotOrigin *= -1.0;
            plane->vpCached = true;
        }

        normalDotDirection = plane->getNormalVector().dotProduct(ray->getDirection());
        if (InfinitePlane::intersectionDepth(plane->vpNormDotOrigin, normalDotDirection,
                GeometryConfig::SMALL_TOLERANCE, GeometryConfig::SMALL_TOLERANCE,
                GeometryConfig::MAX_DISTANCE, false, *depth) == RayFirstHitStatus::Hit) {
            stats.incrementRayPlaneTestsSucceeded();
            return (true);
        }
        return (false);
    }
    normalDotOrigin = plane->getNormalVector().dotProduct(ray->getOrigin());
    normalDotOrigin += plane->getDistance();
    normalDotOrigin *= -1.0;

    normalDotDirection = plane->getNormalVector().dotProduct(ray->getDirection());
    if (InfinitePlane::intersectionDepth(normalDotOrigin, normalDotDirection,
            GeometryConfig::SMALL_TOLERANCE, GeometryConfig::SMALL_TOLERANCE,
            GeometryConfig::MAX_DISTANCE, false, *depth) == RayFirstHitStatus::Hit) {
        stats.incrementRayPlaneTestsSucceeded();
        return (true);
    }
    return (false);
}

int
RayCastingInfinitePlane::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    const RayCastingInfinitePlane *plane = this;

    double temp = testPoint.dotProduct(plane->getNormalVector());
    return ((temp + plane->getDistance()) <= distanceTolerance) ? INSIDE : OUTSIDE;
}

void
RayCastingInfinitePlane::computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    *result = getNormalVector();
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

    const Vector3Dd normal = plane->getNormalVector().multiply(-1.0);
    const double distance = -plane->getDistance();
    plane->halfSpace.getBoundaryPlane().setCoefficients(normal, distance);
    plane->vpCached = false;
}
