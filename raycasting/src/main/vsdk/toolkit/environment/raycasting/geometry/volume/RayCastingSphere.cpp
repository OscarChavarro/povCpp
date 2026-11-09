#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/raycasting/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/IntersectionCandidate.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/RayCastingSphere.h"
#include "java/util/PriorityQueue.txx"

RayCastingSphere::RayCastingSphere() :
    radius(1.0),
    inverted(false)
{
}

RayCastingSphere::RayCastingSphere(bool inverted) :
    radius(1.0),
    inverted(inverted)
{
}

RayCastingSphere::RayCastingSphere(double radius) :
    radius(radius),
    inverted(false)
{
}

RayCastingSphere::RayCastingSphere(double radius, bool inverted) :
    radius(radius),
    inverted(inverted)
{
}

RayCastingSphere::RayCastingSphere(const RayCastingSphere &other) :
    radius(other.radius),
    inverted(other.inverted)
{
}

bool
RayCastingSphere::intersectSphereLocalSpace(
    const Vector3Dd &p,
    const Vector3Dd &d,
    GeometryStatistics *stats,
    double radius,
    double *depth1,
    double *depth2)
{
    stats->incrementRaySphereTests();

    const RayCrossingQuery query(GeometryConfig::SMALL_TOLERANCE,
        GeometryConfig::MAX_DISTANCE, GeometryConfig::SMALL_TOLERANCE,
        GeometryConfig::SMALL_TOLERANCE);
    if (Sphere::intersectionRoots(p, d, radius, query, *depth2, *depth1) == 0) {
        return false;
    }

    stats->incrementRaySphereTestsSucceeded();
    return true;
}

bool
RayCastingSphere::intersectSphere(
    const RayWithTracingState *ray, const RayCastingSphere *sphere,
    double *depth1, double *depth2)
{
    // The caller owns object placement and must transform the ray into object
    // space before invoking this sphere.
    return intersectSphereLocalSpace(
        ray->getOrigin(), ray->getDirection(),
        ray->getGeometryStatistics(), sphere->getRadius(), depth1, depth2);
}

int
RayCastingSphere::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    double depth1;
    double depth2;
    RayCastingSphere * const shape = this;

    if (!RayCastingSphere::intersectSphere(ray, shape, &depth1, &depth2)) {
        return 0;
    }

    IntersectionCandidate localElement;
    localElement.getAttributes().setHitGeometry(shape);
    localElement.getAttributes().setMaterial(materialOverride);

    // Intersection points use the original world-space ray + t (t is invariant
    // under the affine ray-transform because the parameterization is linear).
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

    return 1;
}

int
RayCastingSphere::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    double depth1;
    double depth2;
    RayCastingSphere * const shape = this;

    if (!RayCastingSphere::intersectSphere(ray, shape, &depth1, &depth2)) {
        return false;
    }

    IntersectionCandidate localElement;
    localElement.getAttributes().setHitGeometry(shape);
    localElement.getAttributes().setMaterial(context.getMaterialOverride());
    localElement.getAttributes().pushDetailOwner(context.getDetailOwner());
    localElement.getAttributes().setMaterialUsesObjectLocalPoint(
        context.getMaterialUsesObjectLocalPoint());

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
RayCastingSphere::doIntersectionFirstHit(
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    double depth1;
    double depth2;
    RayCastingSphere * const shape = this;

    if (!RayCastingSphere::intersectSphere(ray, shape, &depth1, &depth2)) {
        return false;
    }

    const double nearestDepth = depth1 < depth2 ? depth1 : depth2;
    out.getIntersection().t = nearestDepth;
    out.getIntersection().point =
        ray->getDirection().multiply(nearestDepth).add(ray->getOrigin());
    out.getAttributes().setHitGeometry(shape);
    out.getAttributes().setMaterial(materialOverride);
    return true;
}

int
RayCastingSphere::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    Vector3Dd q;
    q = testPoint;
    const double distSq = q.dotProduct(q);
    const double radiusSquared = radius * radius;
    if (inverted) {
        return (distSq - radiusSquared > distanceTolerance) ? INSIDE : OUTSIDE;
    }
    return (distSq - radiusSquared < distanceTolerance) ? INSIDE : OUTSIDE;
}

void
RayCastingSphere::computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    // For a sphere at the origin, the local-space normal is the local-space
    // intersection point normalized, regardless of radius.
    *result = intersectionPoint->normalizedFast();
}

void *
RayCastingSphere::copy()
{
    return new RayCastingSphere(*this);
}

void
RayCastingSphere::invertGeometry()
{
    inverted = !inverted;
}

BoundingVolumeHierarchy *
RayCastingSphere::createBoundingVolume() const
{
    return new AabbBoundingVolume(
        Vector3Dd(-radius, -radius, -radius), Vector3Dd(radius, radius, radius));
}
