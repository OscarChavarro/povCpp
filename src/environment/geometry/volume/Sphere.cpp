#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"

#include "common/statistics/Statistics.h"

#include "environment/geometry/Intersection.h"
#include "environment/geometry/volume/Sphere.h"

Sphere::Sphere() :
    Sphere(Vector3Dd(0.0, 0.0, 0.0), 1.0, false)
{
}

Sphere::Sphere(const Vector3Dd &center, double radius, bool inverted) :
    Sphere(center, radius, radius * radius, 1.0 / radius, Vector3Dd(),
        0.0, false, false, inverted)
{
}

Sphere::Sphere(const Vector3Dd &center, double radius, double radiusSquared,
    double inverseRadius, const Vector3Dd &vpOtoC, double vpOCSquared,
    short vpInside, bool vpCached, bool inverted) :
    center(center),
    radius(radius),
    radiusSquared(radiusSquared),
    inverseRadius(inverseRadius),
    vpOtoC(vpOtoC),
    vpOCSquared(vpOCSquared),
    vpInside(vpInside),
    vpCached(vpCached),
    inverted(inverted)
{
}

void
Sphere::updateRadiusState(double radius)
{
    this->radius = radius;
    radiusSquared = radius * radius;
    inverseRadius = 1.0 / radius;
}

int
Sphere::intersectSphere(
    const RayWithSegments *ray, Sphere *sphere, double *depth1, double *depth2)
{
    Statistics &stats = *ray->getStatistics();
    stats.incrementRaySphereTests();

    Vector3Dd originToCenter;
    double ocSquared;
    double tClosestApproach;
    double tHalfChordSquared;
    short inside;

    if (ray->isPrimaryRayEnabled()) {
        if (!sphere->isVpCached()) {
            sphere->getVpOtoC() = sphere->getCenter().subtract(ray->getOrigin());
            sphere->setVpOCSquared(sphere->getVpOtoC().dotProduct(sphere->getVpOtoC()));
            sphere->setVpInside((sphere->getVpOCSquared() < sphere->getRadiusSquared()));
            sphere->setVpCached(true);
        }
        tClosestApproach = sphere->getVpOtoC().dotProduct(ray->getDirection());
        if (!sphere->getVpInside() &&
            (tClosestApproach < GeometryConstants::Small_Tolerance)) {
            return false;
        }
        tHalfChordSquared = sphere->getRadiusSquared() - sphere->getVpOCSquared() +
                            (tClosestApproach * tClosestApproach);
    } else {
        originToCenter = sphere->getCenter().subtract(ray->getOrigin());
        ocSquared = originToCenter.dotProduct(originToCenter);
        inside = (ocSquared < sphere->getRadiusSquared());
        tClosestApproach = originToCenter.dotProduct(ray->getDirection());
        if (!inside && (tClosestApproach < GeometryConstants::Small_Tolerance)) {
            return false;
        }

        tHalfChordSquared = sphere->getRadiusSquared() - ocSquared +
                            (tClosestApproach * tClosestApproach);
    }

    if (tHalfChordSquared < GeometryConstants::Small_Tolerance) {
        return false;
    }

    const double halfChord = java::Math::sqrt(tHalfChordSquared);
    *depth1 = tClosestApproach + halfChord;
    *depth2 = tClosestApproach - halfChord;

    if ((*depth1 < GeometryConstants::Small_Tolerance) || (*depth1 > GeometryConstants::Max_Distance)) {
        if ((*depth2 < GeometryConstants::Small_Tolerance) || (*depth2 > GeometryConstants::Max_Distance)) {
            return false;
        }
        *depth1 = *depth2;

    } else {
        if ((*depth2 < GeometryConstants::Small_Tolerance) || (*depth2 > GeometryConstants::Max_Distance)) {
            *depth2 = *depth1;
        }
    }

    stats.incrementRaySphereTestsSucceeded();
    return true;
}

int
Sphere::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    return allIntersectionsForOwner(
        ray, depthQueue, reinterpret_cast<SimpleBody *>(this));
}

int
Sphere::allIntersectionsForOwner(
    RayWithSegments *ray,
    java::PriorityQueue<Intersection> *depthQueue,
    SimpleBody *owner)
{
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;
    Intersection localElement;
    Sphere * const shape = this;

    bool intersectionFound = false;
    if (Sphere::intersectSphere(ray, shape, &depth1, &depth2)) {
        localElement.setDepth(depth1);
        localElement.setObject(nullptr);
        intersectionPoint = ray->getDirection().multiply(depth1);
        intersectionPoint = intersectionPoint.add(ray->getOrigin());
        localElement.setPoint(intersectionPoint);
        localElement.setShape(owner);
        depthQueue->offer(localElement);
        intersectionFound = true;

        if (depth2 != depth1) {
            localElement.setDepth(depth2);
            localElement.setObject(nullptr);
            intersectionPoint = ray->getDirection().multiply(depth2);
            intersectionPoint = intersectionPoint.add(ray->getOrigin());
            localElement.setPoint(intersectionPoint);
            localElement.setShape(owner);
            depthQueue->offer(localElement);
            intersectionFound = true;
        }
    }
    return intersectionFound;
}

int
Sphere::inside(Vector3Dd *testPoint)
{
    const Sphere *sphere = this;
    const Vector3Dd originToCenter = sphere->getCenter().subtract(*testPoint);
    const double ocSquared = originToCenter.dotProduct(originToCenter);

    if (sphere->isInverted()) {
        return (ocSquared - sphere->getRadiusSquared() > GeometryConstants::Small_Tolerance);
    }
    return (ocSquared - sphere->getRadiusSquared() < GeometryConstants::Small_Tolerance);
}

void
Sphere::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    const Sphere *sphere = this;

    *result = intersectionPoint->subtract(sphere->getCenter());
    *result = (*result).multiply(sphere->getInverseRadius());
}

void *
Sphere::copy()
{
    return new Sphere(*this);
}

void
Sphere::translateGeometry(Vector3Dd *vector)
{
    this->getCenter() = this->getCenter().add(*vector);
}

void
Sphere::rotateGeometry(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    this->getCenter() = transformation.transpose().multiply(this->getCenter());
}

void
Sphere::scaleGeometry(Vector3Dd *vector)
{
    Sphere * const sphere = this;

    if ((vector->x() != vector->y()) || (vector->x() != vector->z())) {
        const double s = (java::Math::abs(vector->x()) + java::Math::abs(vector->y()) + java::Math::abs(vector->z())) / 3.0;
        *vector = Vector3Dd(s, s, s);
    }

    sphere->getCenter() = sphere->getCenter().multiply(vector->x());
    sphere->updateRadiusState(sphere->getRadius() * vector->x());
}

void
Sphere::invertGeometry()
{
    this->toggleInverted();
}

#include "java/util/PriorityQueue.txx"
