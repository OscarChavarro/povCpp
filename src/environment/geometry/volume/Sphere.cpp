#include "java/lang/Math.h"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/Sphere.h"

Sphere::Sphere() :
    inverted(false)
{
}

Sphere::Sphere(bool inverted) :
    inverted(inverted)
{
}

Sphere::Sphere(const Sphere &other) :
    inverted(other.inverted)
{
    if (other.getTransformation() != nullptr) {
        transformation = new Matrix4x4d(*other.getTransformation());
        transformationInverse = new Matrix4x4d(*other.getTransformationInverse());
    }
}

bool
Sphere::intersectSphere(
    const RayWithSegments *ray, const Sphere *sphere,
    double *depth1, double *depth2)
{
    Statistics &stats = *ray->getStatistics();
    stats.incrementRaySphereTests();

    // Transform ray to object space (canonical unit sphere at origin).
    Vector3Dd p, d;
    if (sphere->getTransformation() != nullptr) {
        p = sphere->getTransformationInverse()->transformPoint(ray->getOrigin());
        d = sphere->getTransformationInverse()->transformDirection(ray->getDirection());
    } else {
        p = ray->getOrigin();
        d = ray->getDirection();
    }

    // The object-space direction is NOT unit length: transformPoint/Direction by
    // the inverse of translate(center)*scale(r) leaves |d| = |worldDir| / r. The
    // closed-form chord formula below assumes a unit direction, so normalize d
    // here and convert the resulting (normalized-units) t back to world t by
    // dividing by |d|. Because the ray parameterization is shared between world
    // and object space (affine map), world t == object t; this division recovers
    // it exactly. (Skipping this is what made every sphere with r != 1 render at
    // the wrong size, and made r >~ 1.4 vanish entirely - the discriminant went
    // negative.)
    const double directionLength = java::Math::sqrt(d.dotProduct(d));
    const Vector3Dd unitDirection = d.multiply(1.0 / directionLength);

    // Unit sphere at origin: O2C = -p
    const double ocSquared = p.dotProduct(p);
    const bool inside = (ocSquared < 1.0);
    const double tClosestApproach = -p.dotProduct(unitDirection);

    if (!inside && tClosestApproach < Config::SMALL_TOLERANCE) {
        return false;
    }

    const double tHalfChordSquared = 1.0 - ocSquared + tClosestApproach * tClosestApproach;
    if (tHalfChordSquared < Config::SMALL_TOLERANCE) {
        return false;
    }

    const double halfChord = java::Math::sqrt(tHalfChordSquared);
    *depth1 = (tClosestApproach + halfChord) / directionLength;
    *depth2 = (tClosestApproach - halfChord) / directionLength;

    if ((*depth1 < Config::SMALL_TOLERANCE) || (*depth1 > Config::MAX_DISTANCE)) {
        if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE)) {
            return false;
        }
        *depth1 = *depth2;
    } else if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.incrementRaySphereTestsSucceeded();
    return true;
}

int
Sphere::doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
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
Sphere::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    Vector3Dd q;
    if (getTransformation() != nullptr) {
        q = getTransformationInverse()->transformPoint(testPoint);
    } else {
        q = testPoint;
    }
    const double distSq = q.dotProduct(q);
    if (inverted) {
        return (distSq - 1.0 > distanceTolerance) ? INSIDE : OUTSIDE;
    }
    return (distSq - 1.0 < distanceTolerance) ? INSIDE : OUTSIDE;
}

void
Sphere::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    // For a unit sphere at origin, the object-space normal equals the
    // object-space intersection point (since center = (0,0,0), radius = 1).
    if (getTransformation() != nullptr) {
        *result = getTransformationInverse()->transformPoint(*intersectionPoint);
        *result = getTransformationInverse()->withoutTranslation().multiply(*result);
        *result = result->normalizedFast();
    } else {
        *result = intersectionPoint->normalizedFast();
    }
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

AxisAlignedBox
Sphere::getMinMax() const
{
    if (transformation == nullptr) {
        return AxisAlignedBox{Vector3Dd(-1.0, -1.0, -1.0), Vector3Dd(1.0, 1.0, 1.0)};
    }
    double cx = transformation->get(0, 3);
    double cy = transformation->get(1, 3);
    double cz = transformation->get(2, 3);
    double rx = java::Math::sqrt(
        transformation->get(0,0) * transformation->get(0,0) +
        transformation->get(0,1) * transformation->get(0,1) +
        transformation->get(0,2) * transformation->get(0,2));
    double ry = java::Math::sqrt(
        transformation->get(1,0) * transformation->get(1,0) +
        transformation->get(1,1) * transformation->get(1,1) +
        transformation->get(1,2) * transformation->get(1,2));
    double rz = java::Math::sqrt(
        transformation->get(2,0) * transformation->get(2,0) +
        transformation->get(2,1) * transformation->get(2,1) +
        transformation->get(2,2) * transformation->get(2,2));
    return AxisAlignedBox{
        Vector3Dd(cx - rx, cy - ry, cz - rz),
        Vector3Dd(cx + rx, cy + ry, cz + rz)};
}

#include "java/util/PriorityQueue.txx"
