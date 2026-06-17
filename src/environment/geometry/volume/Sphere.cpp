#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"

#include "common/statistics/Statistics.h"

#include "environment/geometry/volume/Sphere.h"

int
Sphere::intersectSphere(
    const RayWithSegments *ray, Sphere *sphere, double *depth1, double *depth2)
{
    Statistics::global().incrementRaySphereTests();

    Vector3Dd originToCenter;
    double ocSquared;
    double tClosestApproach;
    double halfChord;
    double tHalfChordSquared;
    short inside;

    if (ray->isPrimaryRay) {
        if (!sphere->vpCached) {
            sphere->vpOtoC = sphere->center.subtract(ray->getOrigin());
            sphere->vpOCSquared = sphere->vpOtoC.dotProduct(sphere->vpOtoC);
            sphere->vpInside = (sphere->vpOCSquared < sphere->radiusSquared);
            sphere->vpCached = true;
        }
        tClosestApproach = sphere->vpOtoC.dotProduct(ray->getDirection());
        if (!sphere->vpInside && (tClosestApproach < GeometryConstants::Small_Tolerance)) {
            return false;
        }
        tHalfChordSquared = sphere->radiusSquared - sphere->vpOCSquared +
                            (tClosestApproach * tClosestApproach);
    } else {
        originToCenter = sphere->center.subtract(ray->getOrigin());
        ocSquared = originToCenter.dotProduct(originToCenter);
        inside = (ocSquared < sphere->radiusSquared);
        tClosestApproach = originToCenter.dotProduct(ray->getDirection());
        if (!inside && (tClosestApproach < GeometryConstants::Small_Tolerance)) {
            return false;
        }

        tHalfChordSquared = sphere->radiusSquared - ocSquared +
                            (tClosestApproach * tClosestApproach);
    }

    if (tHalfChordSquared < GeometryConstants::Small_Tolerance) {
        return false;
    }

    halfChord = java::Math::sqrt(tHalfChordSquared);
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

    Statistics::global().incrementRaySphereTestsSucceeded();
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
    bool intersectionFound;
    Sphere * const shape = this;

    intersectionFound = false;
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
    Vector3Dd originToCenter;
    double ocSquared;
    const Sphere *sphere = this;

    originToCenter = sphere->center.subtract(*testPoint);
    ocSquared = originToCenter.dotProduct(originToCenter);

    if (sphere->inverted) {
        return (ocSquared - sphere->radiusSquared > GeometryConstants::Small_Tolerance);
    }
    return (ocSquared - sphere->radiusSquared < GeometryConstants::Small_Tolerance);
}

void
Sphere::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    const Sphere *sphere = this;

    *result = intersectionPoint->subtract(sphere->center);
    *result = (*result).multiply(sphere->inverseRadius);
}

void *
Sphere::copy()
{
    Sphere *newShape;

    newShape = new Sphere;
    *newShape = *this;

    return (newShape);
}

void
Sphere::translateGeometry(Vector3Dd *vector)
{
    this->center = this->center.add(*vector);
}

void
Sphere::rotateGeometry(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    this->center = transformation.transpose().multiply(this->center);
}

void
Sphere::scaleGeometry(Vector3Dd *vector)
{
    Sphere * const sphere = this;

    if ((vector->x() != vector->y()) || (vector->x() != vector->z())) {
        const double s = (java::Math::abs(vector->x()) + java::Math::abs(vector->y()) + java::Math::abs(vector->z())) / 3.0;
        *vector = Vector3Dd(s, s, s);
    }

    sphere->center = sphere->center.multiply(vector->x());
    sphere->radius *= vector->x();
    sphere->radiusSquared = sphere->radius * sphere->radius;
    sphere->inverseRadius = 1.0 / sphere->radius;
}

void
Sphere::invertGeometry()
{
    this->inverted ^= true;
}

#include "java/util/PriorityQueue.txx"
