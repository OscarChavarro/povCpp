/****************************************************************************
 *                     planes.c
 *
 *  This module implements functions that manipulate planes.
 *
 *****************************************************************************/

#include "geom/Planes.h"
#include "io/Parse.h"
#include "geom/Objects.h"
#include "io/Parse.h"
Methods Plane_Methods = {Composite::objectIntersect, InfinitePlane::allPlaneIntersections,
    InfinitePlane::insidePlane, InfinitePlane::planeNormal, InfinitePlane::copyPlane, InfinitePlane::translatePlane, InfinitePlane::rotatePlane,
    InfinitePlane::scalePlane, InfinitePlane::invertPlane};


extern Ray *vpRay;
extern long rayPlaneTests, rayPlaneTestsSucceeded;
int
InfinitePlane::allPlaneIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    InfinitePlane *shape = (InfinitePlane *)object;
    double depth;
    Vector3D intersectionPoint;
    Intersection localElement;

    if (InfinitePlane::intersectPlane(ray, shape, &depth)) {
        if (depth > Small_Tolerance) {
            localElement.Depth = depth;
            localElement.Object = shape->Parent_Object;
            VectorOps::vScale(intersectionPoint, ray->Direction, depth);
            VectorOps::vAdd(intersectionPoint, intersectionPoint, ray->Initial);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            return (TRUE);
        }
    }

    return (FALSE);
}

int
InfinitePlane::intersectPlane(Ray *ray, InfinitePlane *plane, double *depth)
{
    double normalDotOrigin, normalDotDirection;

    rayPlaneTests++;
    if (ray == vpRay) {
        if (!plane->VPCached) {
            VectorOps::vDot(plane->VPNormDotOrigin, plane->Normal_Vector, ray->Initial);
            plane->VPNormDotOrigin += plane->Distance;
            plane->VPNormDotOrigin *= -1.0;
            plane->VPCached = TRUE;
        }

        VectorOps::vDot(normalDotDirection, plane->Normal_Vector, ray->Direction);
        if ((normalDotDirection < Small_Tolerance) &&
            (normalDotDirection > -Small_Tolerance)) {
            return (FALSE);
        }

        *depth = plane->VPNormDotOrigin / normalDotDirection;
        if ((*depth >= Small_Tolerance) && (*depth <= Max_Distance)) {
            rayPlaneTestsSucceeded++;
            return (TRUE);
        }
        return (FALSE);
    }
    VectorOps::vDot(normalDotOrigin, plane->Normal_Vector, ray->Initial);
    normalDotOrigin += plane->Distance;
    normalDotOrigin *= -1.0;

    VectorOps::vDot(normalDotDirection, plane->Normal_Vector, ray->Direction);
    if ((normalDotDirection < Small_Tolerance) &&
        (normalDotDirection > -Small_Tolerance)) {
        return (FALSE);
    }

    *depth = normalDotOrigin / normalDotDirection;
    if ((*depth >= Small_Tolerance) && (*depth <= Max_Distance)) {
        rayPlaneTestsSucceeded++;
        return (TRUE);
    }
    return (FALSE);
}

int
InfinitePlane::insidePlane(Vector3D *testPoint, SimpleBody *object)
{
    InfinitePlane *plane = (InfinitePlane *)object;
    double temp;

    VectorOps::vDot(temp, *testPoint, plane->Normal_Vector);
    return ((temp + plane->Distance) <= Small_Tolerance);
}

void
InfinitePlane::planeNormal(Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    InfinitePlane *plane = (InfinitePlane *)object;

    *result = plane->Normal_Vector;
}

void *
InfinitePlane::copyPlane(SimpleBody *object)
{
    InfinitePlane *newShape;

    newShape = ParseFactory::getPlaneShape();
    *newShape = *((InfinitePlane *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = ParseEngine::copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
InfinitePlane::translatePlane(SimpleBody *object, Vector3D *vector)
{
    InfinitePlane *plane = (InfinitePlane *)object;
    Vector3D translation;

    VectorOps::vEvaluate(translation, plane->Normal_Vector, *vector);
    plane->Distance -= translation.x + translation.y + translation.z;

    TextureUtils::translateTexture(&plane->Shape_Texture, vector);
}

void
InfinitePlane::rotatePlane(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;

    Transformation::getRotationTransformation(&transformation, vector);
    Transformation::MTransformVector(&((InfinitePlane *)object)->Normal_Vector,
        &((InfinitePlane *)object)->Normal_Vector, &transformation);

    TextureUtils::rotateTexture(&((InfinitePlane *)object)->Shape_Texture, vector);
}

void
InfinitePlane::scalePlane(SimpleBody *object, Vector3D *vector)
{
    double length;
    InfinitePlane *plane = (InfinitePlane *)object;

    plane->Normal_Vector.x = plane->Normal_Vector.x / vector->x;
    plane->Normal_Vector.y = plane->Normal_Vector.y / vector->y;
    plane->Normal_Vector.z = plane->Normal_Vector.z / vector->z;

    VectorOps::vLength(length, plane->Normal_Vector);
    VectorOps::vScale(plane->Normal_Vector, plane->Normal_Vector, 1.0 / length);
    plane->Distance /= length;

    TextureUtils::scaleTexture(&((InfinitePlane *)object)->Shape_Texture, vector);
}

void
InfinitePlane::invertPlane(SimpleBody *object)
{
    InfinitePlane *plane = (InfinitePlane *)object;

    VectorOps::vScale(plane->Normal_Vector, plane->Normal_Vector, -1.0);
    plane->Distance *= -1.0;
}
