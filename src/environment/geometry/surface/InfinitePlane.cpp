/****************************************************************************
 *                     planes.c
 *
 *  This module implements functions that manipulate planes.
 *
 *****************************************************************************/

#include "environment/geometry/surface/InfinitePlane.h"
#include "common/Statistics.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/volume/compound/Composite.h"
Methods InfinitePlane::methodTable = {Composite::objectIntersect,
    InfinitePlane::allPlaneIntersections, InfinitePlane::insidePlane,
    InfinitePlane::planeNormal, InfinitePlane::copyPlane,
    InfinitePlane::translatePlane, InfinitePlane::rotatePlane,
    InfinitePlane::scalePlane, InfinitePlane::invertPlane};

int
InfinitePlane::allPlaneIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    InfinitePlane *shape = (InfinitePlane *)object;
    double depth;
    Vector3Dd intersectionPoint;
    Intersection localElement;

    if (InfinitePlane::intersectPlane(ray, shape, &depth)) {
        if (depth > GeometryConstants::Small_Tolerance) {
            localElement.Depth = depth;
            localElement.Object = nullptr;
            VectorOps::vScale(intersectionPoint, ray->direction, depth);
            intersectionPoint.add(ray->position);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            return (LegacyBoolean::TRUE_VALUE);
        }
    }

    return (LegacyBoolean::FALSE_VALUE);
}

int
InfinitePlane::intersectPlane(
    RayWithSegments *ray, InfinitePlane *plane, double *depth)
{
    double normalDotOrigin;
    double normalDotDirection;

    Statistics::global().rayPlaneTests++;
    if (ray->isPrimaryRay) {
        if (!plane->VPCached) {
            plane->VPNormDotOrigin =
                plane->normalVector.dotProduct(ray->position);
            plane->VPNormDotOrigin += plane->Distance;
            plane->VPNormDotOrigin *= -1.0;
            plane->VPCached = LegacyBoolean::TRUE_VALUE;
        }

        normalDotDirection = plane->normalVector.dotProduct(ray->direction);
        if ((normalDotDirection < GeometryConstants::Small_Tolerance) &&
            (normalDotDirection > -GeometryConstants::Small_Tolerance)) {
            return (LegacyBoolean::FALSE_VALUE);
        }

        *depth = plane->VPNormDotOrigin / normalDotDirection;
        if ((*depth >= GeometryConstants::Small_Tolerance) && (*depth <= GeometryConstants::Max_Distance)) {
            Statistics::global().rayPlaneTestsSucceeded++;
            return (LegacyBoolean::TRUE_VALUE);
        }
        return (LegacyBoolean::FALSE_VALUE);
    }
    normalDotOrigin = plane->normalVector.dotProduct(ray->position);
    normalDotOrigin += plane->Distance;
    normalDotOrigin *= -1.0;

    normalDotDirection = plane->normalVector.dotProduct(ray->direction);
    if ((normalDotDirection < GeometryConstants::Small_Tolerance) &&
        (normalDotDirection > -GeometryConstants::Small_Tolerance)) {
        return (LegacyBoolean::FALSE_VALUE);
    }

    *depth = normalDotOrigin / normalDotDirection;
    if ((*depth >= GeometryConstants::Small_Tolerance) && (*depth <= GeometryConstants::Max_Distance)) {
        Statistics::global().rayPlaneTestsSucceeded++;
        return (LegacyBoolean::TRUE_VALUE);
    }
    return (LegacyBoolean::FALSE_VALUE);
}

int
InfinitePlane::insidePlane(Vector3Dd *testPoint, SimpleBody *object)
{
    InfinitePlane *plane = (InfinitePlane *)object;
    double temp;

    temp = (*testPoint).dotProduct(plane->normalVector);
    return ((temp + plane->Distance) <= GeometryConstants::Small_Tolerance);
}

void
InfinitePlane::planeNormal(
    Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint)
{
    InfinitePlane *plane = (InfinitePlane *)object;

    *result = plane->normalVector;
}

void *
InfinitePlane::copyPlane(SimpleBody *object)
{
    InfinitePlane *newShape;

    newShape = new InfinitePlane;
    *newShape = *((InfinitePlane *)object);
    newShape->nextObject = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture =
            TextureUtils::copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
InfinitePlane::translatePlane(SimpleBody *object, Vector3Dd *vector)
{
    InfinitePlane *plane = (InfinitePlane *)object;
    Vector3Dd translation;

    VectorOps::vEvaluate(translation, plane->normalVector, *vector);
    plane->Distance -= translation.x + translation.y + translation.z;

    TextureUtils::translateTexture(&plane->Shape_Texture, vector);
}

void
InfinitePlane::rotatePlane(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;

    Transformation::getRotationTransformation(&transformation, vector);
    Transformation::MTransformVector(&((InfinitePlane *)object)->normalVector,
        &((InfinitePlane *)object)->normalVector, &transformation);

    TextureUtils::rotateTexture(
        &((InfinitePlane *)object)->Shape_Texture, vector);
}

void
InfinitePlane::scalePlane(SimpleBody *object, Vector3Dd *vector)
{
    double length;
    InfinitePlane *plane = (InfinitePlane *)object;

    plane->normalVector.x = plane->normalVector.x / vector->x;
    plane->normalVector.y = plane->normalVector.y / vector->y;
    plane->normalVector.z = plane->normalVector.z / vector->z;

    length = plane->normalVector.length();
    plane->normalVector.scale(1.0 / length);
    plane->Distance /= length;

    TextureUtils::scaleTexture(
        &((InfinitePlane *)object)->Shape_Texture, vector);
}

void
InfinitePlane::invertPlane(SimpleBody *object)
{
    InfinitePlane *plane = (InfinitePlane *)object;

    plane->normalVector.scale(-1.0);
    plane->Distance *= -1.0;
}
