#include "java/lang/Math.h"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/common/statistics/GeometryStatistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/Sphere.h"

Sphere::Sphere() :
    radius(1.0),
    inverted(false)
{
}

Sphere::Sphere(bool inverted) :
    radius(1.0),
    inverted(inverted)
{
}

Sphere::Sphere(double radius) :
    radius(radius),
    inverted(false)
{
}

Sphere::Sphere(double radius, bool inverted) :
    radius(radius),
    inverted(inverted)
{
}

Sphere::Sphere(const Sphere &other) :
    radius(other.radius),
    inverted(other.inverted)
{
}

bool
Sphere::intersectSphereLocalSpace(
    const Vector3Dd &p,
    const Vector3Dd &d,
    GeometryStatistics *stats,
    double radius,
    double *depth1,
    double *depth2)
{
    stats->incrementRaySphereTests();

    // The object-space direction is NOT unit length whenever the caller's
    // remaining transform (rotation composed with any explicit scale) is
    // non-isotropic: |d| = |worldDir| / scale. The closed-form chord formula
    // below assumes a unit direction, so normalize d here and convert the
    // resulting (normalized-units) t back to world t by dividing by |d|.
    // Because the ray parameterization is shared between world and object
    // space (affine map), world t == object t; this division recovers it
    // exactly. (Skipping this is what made every sphere with r != 1 render at
    // the wrong size, and made r >~ 1.4 vanish entirely - the discriminant went
    // negative.)
    const double directionLength = java::Math::sqrt(d.dotProduct(d));
    const Vector3Dd unitDirection = d.multiply(1.0 / directionLength);

    // Sphere of the given radius at the origin: O2C = -p
    const double radiusSquared = radius * radius;
    const double ocSquared = p.dotProduct(p);
    const bool inside = (ocSquared < radiusSquared);
    const double tClosestApproach = -p.dotProduct(unitDirection);

    if (!inside && tClosestApproach < GeometryConfig::SMALL_TOLERANCE) {
        return false;
    }

    const double tHalfChordSquared =
        radiusSquared - ocSquared + tClosestApproach * tClosestApproach;
    if (tHalfChordSquared < GeometryConfig::SMALL_TOLERANCE) {
        return false;
    }

    const double halfChord = java::Math::sqrt(tHalfChordSquared);
    *depth1 = (tClosestApproach + halfChord) / directionLength;
    *depth2 = (tClosestApproach - halfChord) / directionLength;

    if ((*depth1 < GeometryConfig::SMALL_TOLERANCE) || (*depth1 > GeometryConfig::MAX_DISTANCE)) {
        if ((*depth2 < GeometryConfig::SMALL_TOLERANCE) || (*depth2 > GeometryConfig::MAX_DISTANCE)) {
            return false;
        }
        *depth1 = *depth2;
    } else if ((*depth2 < GeometryConfig::SMALL_TOLERANCE) || (*depth2 > GeometryConfig::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats->incrementRaySphereTestsSucceeded();
    return true;
}

bool
Sphere::intersectSphere(
    const RayWithTracingState *ray, const Sphere *sphere,
    double *depth1, double *depth2)
{
    // The caller owns object placement and must transform the ray into object
    // space before invoking this sphere.
    return intersectSphereLocalSpace(
        ray->getOrigin(), ray->getDirection(),
        ray->getGeometryStatistics(), sphere->getRadius(), depth1, depth2);
}

int
Sphere::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    double depth1;
    double depth2;
    Sphere * const shape = this;

    if (!Sphere::intersectSphere(ray, shape, &depth1, &depth2)) {
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
Sphere::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    double depth1;
    double depth2;
    Sphere * const shape = this;

    if (!Sphere::intersectSphere(ray, shape, &depth1, &depth2)) {
        return false;
    }

    IntersectionCandidate localElement;
    localElement.getAttributes().setHitGeometry(shape);
    localElement.getAttributes().setMaterial(context.materialOverride);
    localElement.getAttributes().pushDetailOwner(context.detailOwner);
    localElement.getAttributes().setMaterialUsesObjectLocalPoint(
        context.materialUsesObjectLocalPoint);

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
Sphere::doIntersectionFirstHit(
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    double depth1;
    double depth2;
    Sphere * const shape = this;

    if (!Sphere::intersectSphere(ray, shape, &depth1, &depth2)) {
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
Sphere::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
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
Sphere::computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    // For a sphere at the origin, the local-space normal is the local-space
    // intersection point normalized, regardless of radius.
    *result = intersectionPoint->normalizedFast();
}

void *
Sphere::copy()
{
    return new Sphere(*this);
}

void
Sphere::invertGeometry()
{
    inverted = !inverted;
}

AxisAlignedBoundingBox
Sphere::getMinMax() const
{
    return AxisAlignedBoundingBox{
        Vector3Dd(-radius, -radius, -radius), Vector3Dd(radius, radius, radius)};
}
