/**
planes.c

This module implements functions that manipulate planes.
*/

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/material/MaterialUtils.h"

int
InfinitePlane::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    InfinitePlane * const shape = this;
    double depth;
    Vector3Dd intersectionPoint;
    Intersection localElement;

    if (InfinitePlane::intersectPlane(ray, shape, &depth)) {
        if (depth > GeometryConstants::Small_Tolerance) {
            localElement.depth = depth;
            localElement.Object = nullptr;
            intersectionPoint = ray->direction.multiply(depth);
            intersectionPoint = intersectionPoint.add(ray->position);
            localElement.point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->offer(localElement);
            return (true);
        }
    }

    return (false);
}

int
InfinitePlane::intersectPlane(
    RayWithSegments *ray, InfinitePlane *plane, double *depth)
{
    double normalDotOrigin;
    double normalDotDirection;

    Statistics::global().rayPlaneTests++;
    if (ray->isPrimaryRay) {
        if (!plane->vpCached) {
            plane->vpNormDotOrigin =
                plane->normalVector.dotProduct(ray->position);
            plane->vpNormDotOrigin += plane->distance;
            plane->vpNormDotOrigin *= -1.0;
            plane->vpCached = true;
        }

        normalDotDirection = plane->normalVector.dotProduct(ray->direction);
        if ((normalDotDirection < GeometryConstants::Small_Tolerance) &&
            (normalDotDirection > -GeometryConstants::Small_Tolerance)) {
            return (false);
        }

        *depth = plane->vpNormDotOrigin / normalDotDirection;
        if ((*depth >= GeometryConstants::Small_Tolerance) && (*depth <= GeometryConstants::Max_Distance)) {
            Statistics::global().rayPlaneTestsSucceeded++;
            return (true);
        }
        return (false);
    }
    normalDotOrigin = plane->normalVector.dotProduct(ray->position);
    normalDotOrigin += plane->distance;
    normalDotOrigin *= -1.0;

    normalDotDirection = plane->normalVector.dotProduct(ray->direction);
    if ((normalDotDirection < GeometryConstants::Small_Tolerance) &&
        (normalDotDirection > -GeometryConstants::Small_Tolerance)) {
        return (false);
    }

    *depth = normalDotOrigin / normalDotDirection;
    if ((*depth >= GeometryConstants::Small_Tolerance) && (*depth <= GeometryConstants::Max_Distance)) {
        Statistics::global().rayPlaneTestsSucceeded++;
        return (true);
    }
    return (false);
}

int
InfinitePlane::inside(Vector3Dd *testPoint)
{
    const InfinitePlane *plane = this;
    double temp;

    temp = (*testPoint).dotProduct(plane->normalVector);
    return ((temp + plane->distance) <= GeometryConstants::Small_Tolerance);
}

void
InfinitePlane::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    const InfinitePlane *plane = this;

    *result = plane->normalVector;
}

void *
InfinitePlane::copy()
{
    InfinitePlane *newShape;

    newShape = new InfinitePlane;
    *newShape = *this;

    if (newShape->material != nullptr) {
        newShape->material =
            MaterialUtils::instance().copyTexture(newShape->material);
    }

    return (newShape);
}

void
InfinitePlane::translate(Vector3Dd *vector)
{
    InfinitePlane * const plane = this;
    Vector3Dd translation;

    translation = plane->normalVector.multiply(*vector);
    plane->distance -= translation.x() + translation.y() + translation.z();

    MaterialUtils::instance().translateTexture(&plane->material, vector);
}

void
InfinitePlane::rotate(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    this->normalVector = transformation.transpose().multiply(this->normalVector);

    MaterialUtils::instance().rotateTexture(&this->material, vector);
}

void
InfinitePlane::scale(Vector3Dd *vector)
{
    double length;
    InfinitePlane * const plane = this;

    plane->normalVector = Vector3Dd(
        plane->normalVector.x() / vector->x(),
        plane->normalVector.y() / vector->y(),
        plane->normalVector.z() / vector->z());

    length = plane->normalVector.length();
    plane->normalVector = plane->normalVector.multiply(1.0 / length);
    plane->distance /= length;

    MaterialUtils::instance().scaleTexture(&this->material, vector);
}

void
InfinitePlane::invert()
{
    InfinitePlane * const plane = this;

    plane->normalVector = plane->normalVector.multiply(-1.0);
    plane->distance *= -1.0;
}
#include "java/util/PriorityQueue.txx"
