/****************************************************************************
 *                     planes.c
 *
 *  This module implements functions that manipulate planes.
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 *****************************************************************************/

#include "geom/Planes.h"
#include "geom/Objects.h"

/*===========================================================================*/

Methods Plane_Methods = {objectIntersect, allPlaneIntersections,
    insidePlane, planeNormal, copyPlane, translatePlane, rotatePlane,
    scalePlane, invertPlane};

extern InfinitePlane *getPlaneShape();

extern Ray *vpRay;
extern long rayPlaneTests, rayPlaneTestsSucceeded;

/*===========================================================================*/

int
allPlaneIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    InfinitePlane *shape = (InfinitePlane *)object;
    DBL depth;
    Vector3D intersectionPoint;
    Intersection localElement;

    if (intersectPlane(ray, shape, &depth)) {
        if (depth > Small_Tolerance) {
            localElement.Depth = depth;
            localElement.Object = shape->Parent_Object;
            VScale(intersectionPoint, ray->Direction, depth);
            VAdd(intersectionPoint, intersectionPoint, ray->Initial);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            return (TRUE);
        }
    }

    return (FALSE);
}

int
intersectPlane(Ray *ray, InfinitePlane *plane, DBL *depth)
{
    DBL normalDotOrigin, normalDotDirection;

    rayPlaneTests++;
    if (ray == vpRay) {
        if (!plane->VPCached) {
            VDot(plane->VPNormDotOrigin, plane->Normal_Vector, ray->Initial);
            plane->VPNormDotOrigin += plane->Distance;
            plane->VPNormDotOrigin *= -1.0;
            plane->VPCached = TRUE;
        }

        VDot(normalDotDirection, plane->Normal_Vector, ray->Direction);
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
    VDot(normalDotOrigin, plane->Normal_Vector, ray->Initial);
    normalDotOrigin += plane->Distance;
    normalDotOrigin *= -1.0;

    VDot(normalDotDirection, plane->Normal_Vector, ray->Direction);
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
insidePlane(Vector3D *testPoint, SimpleBody *object)
{
    InfinitePlane *plane = (InfinitePlane *)object;
    DBL temp;

    VDot(temp, *testPoint, plane->Normal_Vector);
    return ((temp + plane->Distance) <= Small_Tolerance);
}

void
planeNormal(Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    InfinitePlane *plane = (InfinitePlane *)object;

    *result = plane->Normal_Vector;
}

void *
copyPlane(SimpleBody *object)
{
    InfinitePlane *newShape;

    newShape = getPlaneShape();
    *newShape = *((InfinitePlane *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
translatePlane(SimpleBody *object, Vector3D *vector)
{
    InfinitePlane *plane = (InfinitePlane *)object;
    Vector3D translation;

    VEvaluate(translation, plane->Normal_Vector, *vector);
    plane->Distance -= translation.x + translation.y + translation.z;

    translateTexture(&plane->Shape_Texture, vector);
}

void
rotatePlane(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;

    getRotationTransformation(&transformation, vector);
    MTransformVector(&((InfinitePlane *)object)->Normal_Vector,
        &((InfinitePlane *)object)->Normal_Vector, &transformation);

    rotateTexture(&((InfinitePlane *)object)->Shape_Texture, vector);
}

void
scalePlane(SimpleBody *object, Vector3D *vector)
{
    DBL length;
    InfinitePlane *plane = (InfinitePlane *)object;

    plane->Normal_Vector.x = plane->Normal_Vector.x / vector->x;
    plane->Normal_Vector.y = plane->Normal_Vector.y / vector->y;
    plane->Normal_Vector.z = plane->Normal_Vector.z / vector->z;

    VLength(length, plane->Normal_Vector);
    VScale(plane->Normal_Vector, plane->Normal_Vector, 1.0 / length);
    plane->Distance /= length;

    scaleTexture(&((InfinitePlane *)object)->Shape_Texture, vector);
}

void
invertPlane(SimpleBody *object)
{
    InfinitePlane *plane = (InfinitePlane *)object;

    VScale(plane->Normal_Vector, plane->Normal_Vector, -1.0);
    plane->Distance *= -1.0;
}
