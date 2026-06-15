#include "java/lang/Math.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/material/Material.h"
#include "environment/material/MaterialUtils.h"

int
Sphere::intersectSphere(
    const RayWithSegments *ray, Sphere *sphere, double *depth1, double *depth2)
{
    Statistics::global().raySphereTests++;

    Vector3Dd originToCenter;
    double ocSquared;
    double tClosestApproach;
    double halfChord;
    double tHalfChordSquared;
    short inside;

    if (ray->isPrimaryRay) {
        if (!sphere->VPCached) {
            sphere->vpOtoC = sphere->center.subtract(ray->position);
            sphere->VPOCSquared = sphere->vpOtoC.dotProduct(sphere->vpOtoC);
            sphere->VPinside = (sphere->VPOCSquared < sphere->radiusSquared);
            sphere->VPCached = true;
        }
        tClosestApproach = sphere->vpOtoC.dotProduct(ray->direction);
        if (!sphere->VPinside && (tClosestApproach < GeometryConstants::Small_Tolerance)) {
            return false;
        }
        tHalfChordSquared = sphere->radiusSquared - sphere->VPOCSquared +
                            (tClosestApproach * tClosestApproach);
    } else {
        originToCenter = sphere->center.subtract(ray->position);
        ocSquared = originToCenter.dotProduct(originToCenter);
        inside = (ocSquared < sphere->radiusSquared);
        tClosestApproach = originToCenter.dotProduct(ray->direction);
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

    Statistics::global().raySphereTestsSucceeded++;
    return true;
}

int
Sphere::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;
    Intersection localElement;
    bool intersectionFound;
    Sphere * const shape = this;

    intersectionFound = false;
    if (Sphere::intersectSphere(ray, shape, &depth1, &depth2)) {
        localElement.Depth = depth1;
        localElement.Object = nullptr;
        intersectionPoint = ray->direction.multiply(depth1);
        intersectionPoint = intersectionPoint.add(ray->position);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->offer(localElement);
        intersectionFound = true;

        if (depth2 != depth1) {
            localElement.Depth = depth2;
            localElement.Object = nullptr;
            intersectionPoint = ray->direction.multiply(depth2);
            intersectionPoint = intersectionPoint.add(ray->position);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
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

    if (newShape->material != nullptr) {
        newShape->material =
            MaterialUtils::instance().copyTexture(newShape->material);
    }

    return (newShape);
}

void
Sphere::translate(Vector3Dd *vector)
{
    this->center = this->center.add(*vector);
    MaterialUtils::instance().translateTexture(&this->material, vector);
}

void
Sphere::rotate(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    this->center = transformation.transpose().multiply(this->center);
    MaterialUtils::instance().rotateTexture(&this->material, vector);
}

void
Sphere::scale(Vector3Dd *vector)
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
    MaterialUtils::instance().scaleTexture(&this->material, vector);
}

void
Sphere::invert()
{
    this->inverted ^= true;
}
#include "java/util/PriorityQueue.txx"
