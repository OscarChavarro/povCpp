/****************************************************************************
 *                     spheres.c
 *
 *  This module implements the sphere primitive.
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
 *****************************************************************************/

#include "geom/Spheres.h"
#include "geom/Objects.h"

//===========================================================================

Methods Sphere_Methods = {objectIntersect, allSphereIntersections,
    insideSphere, sphereNormal, copySphere, translateSphere, rotateSphere,
    scaleSphere, invertSphere};

extern Sphere *getSphereShape();
extern Ray *vpRay;
extern long raySphereTests, raySphereTestsSucceeded;

//===========================================================================

/**
Study closely this method!
*/
int
intersectSphere(Ray *ray, Sphere *sphere, DBL *depth1, DBL *depth2)
{
    raySphereTests++;

    //--------------------------------------------------------------------------
    Vector3D originToCenter;
    DBL ocSquared, tClosestApproach, halfChord, tHalfChordSquared;
    short inside;

    if (ray == vpRay) {
        if (!sphere->VPCached) {
            VSub(sphere->VPOtoC, sphere->Center, ray->Initial);
            VDot(sphere->VPOCSquared, sphere->VPOtoC, sphere->VPOtoC);
            sphere->VPinside = (sphere->VPOCSquared < sphere->Radius_Squared);
            sphere->VPCached = TRUE;
        }
        VDot(tClosestApproach, sphere->VPOtoC, ray->Direction);
        if (!sphere->VPinside && (tClosestApproach < Small_Tolerance)) {
            return FALSE;
        }
        tHalfChordSquared = sphere->Radius_Squared - sphere->VPOCSquared +
                            (tClosestApproach * tClosestApproach);
    } else {
        VSub(originToCenter, sphere->Center, ray->Initial);
        VDot(ocSquared, originToCenter, originToCenter);
        inside = (ocSquared < sphere->Radius_Squared);
        VDot(tClosestApproach, originToCenter, ray->Direction);
        if (!inside && (tClosestApproach < Small_Tolerance)) {
            return FALSE;
        }

        tHalfChordSquared = sphere->Radius_Squared - ocSquared +
                            (tClosestApproach * tClosestApproach);
    }

    if (tHalfChordSquared < Small_Tolerance) {
        return FALSE;
    }

    halfChord = sqrt(tHalfChordSquared);
    *depth1 = tClosestApproach + halfChord;
    *depth2 = tClosestApproach - halfChord;

    if ((*depth1 < Small_Tolerance) || (*depth1 > Max_Distance)) {
        if ((*depth2 < Small_Tolerance) || (*depth2 > Max_Distance)) {
            return FALSE;
        }
        *depth1 = *depth2;

    } else {
        if ((*depth2 < Small_Tolerance) || (*depth2 > Max_Distance)) {
            *depth2 = *depth1;
        }
    }

    //--------------------------------------------------------------------------
    raySphereTestsSucceeded++;
    return TRUE;
}

int
allSphereIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    DBL depth1, depth2;
    Vector3D intersectionPoint;
    Intersection localElement;
    register int intersectionFound;
    Sphere *shape = (Sphere *)object;

    intersectionFound = FALSE;
    if (intersectSphere(ray, shape, &depth1, &depth2)) {
        localElement.Depth = depth1;
        localElement.Object = shape->Parent_Object;
        VScale(intersectionPoint, ray->Direction, depth1);
        VAdd(intersectionPoint, intersectionPoint, ray->Initial);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = TRUE;

        if (depth2 != depth1) {
            localElement.Depth = depth2;
            localElement.Object = shape->Parent_Object;
            VScale(intersectionPoint, ray->Direction, depth2);
            VAdd(intersectionPoint, intersectionPoint, ray->Initial);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            intersectionFound = TRUE;
        }
    }
    return intersectionFound;
}

int
insideSphere(Vector3D *testPoint, SimpleBody *object)
{
    Vector3D originToCenter;
    DBL ocSquared;
    Sphere *sphere = (Sphere *)object;

    VSub(originToCenter, sphere->Center, *testPoint);
    VDot(ocSquared, originToCenter, originToCenter);

    if (sphere->Inverted) {
        return (ocSquared - sphere->Radius_Squared > Small_Tolerance);
    }
    return (ocSquared - sphere->Radius_Squared < Small_Tolerance);
}

void
sphereNormal(Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    Sphere *sphere = (Sphere *)object;

    VSub(*result, *intersectionPoint, sphere->Center);
    VScale(*result, *result, sphere->Inverse_Radius);
}

void *
copySphere(SimpleBody *object)
{
    Sphere *newShape;

    newShape = getSphereShape();
    *newShape = *((Sphere *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
translateSphere(SimpleBody *object, Vector3D *vector)
{
    VAdd(((Sphere *)object)->Center, ((Sphere *)object)->Center, *vector);
    translateTexture(&((Sphere *)object)->Shape_Texture, vector);
}

void
rotateSphere(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;

    getRotationTransformation(&transformation, vector);
    MTransformVector(&((Sphere *)object)->Center, &((Sphere *)object)->Center,
        &transformation);
    rotateTexture(&((Sphere *)object)->Shape_Texture, vector);
}

void
scaleSphere(SimpleBody *object, Vector3D *vector)
{
    Sphere *sphere = (Sphere *)object;

    if ((vector->x != vector->y) || (vector->x != vector->z)) {
        fprintf(stderr, "Error - you cannot scale a sphere unevenly\n");
        exit(1);
    }

    VScale(sphere->Center, sphere->Center, vector->x);
    sphere->Radius *= vector->x;
    sphere->Radius_Squared = sphere->Radius * sphere->Radius;
    sphere->Inverse_Radius = 1.0 / sphere->Radius;
    scaleTexture(&((Sphere *)object)->Shape_Texture, vector);
}

void
invertSphere(SimpleBody *object)
{
    ((Sphere *)object)->Inverted ^= TRUE;
}

//===========================================================================
//= EOF =
//===========================================================================
