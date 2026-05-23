/****************************************************************************
 *                     spheres.c
 *
 *  This module implements the sphere primitive.
 * *****************************************************************************/

#include "environment/geometry/volume/Sphere.h"
#include "common/Statistics.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "media/Texture.h"

//===========================================================================

Methods Sphere_Methods = {Composite::objectIntersect,
    Sphere::allSphereIntersections, Sphere::insideSphere, Sphere::sphereNormal,
    Sphere::copySphere, Sphere::translateSphere, Sphere::rotateSphere,
    Sphere::scaleSphere, Sphere::invertSphere};

extern RayWithSegments *vpRay;

//===========================================================================

/**
Study closely this method!
*/
int
Sphere::intersectSphere(
    RayWithSegments *ray, Sphere *sphere, double *depth1, double *depth2)
{
    globalStatistics.raySphereTests++;

    //--------------------------------------------------------------------------
    Vector3Dd originToCenter;
    double ocSquared;
    double tClosestApproach;
    double halfChord;
    double tHalfChordSquared;
    short inside;

    if (ray == vpRay) {
        if (!sphere->VPCached) {
            VectorOps::vSub(sphere->VPOtoC, sphere->Center, ray->position);
            sphere->VPOCSquared = sphere->VPOtoC.dotProduct(sphere->VPOtoC);
            sphere->VPinside = (sphere->VPOCSquared < sphere->radiusSquared);
            sphere->VPCached = TRUE;
        }
        tClosestApproach = sphere->VPOtoC.dotProduct(ray->direction);
        if (!sphere->VPinside && (tClosestApproach < Small_Tolerance)) {
            return FALSE;
        }
        tHalfChordSquared = sphere->radiusSquared - sphere->VPOCSquared +
                            (tClosestApproach * tClosestApproach);
    } else {
        VectorOps::vSub(originToCenter, sphere->Center, ray->position);
        ocSquared = originToCenter.dotProduct(originToCenter);
        inside = (ocSquared < sphere->radiusSquared);
        tClosestApproach = originToCenter.dotProduct(ray->direction);
        if (!inside && (tClosestApproach < Small_Tolerance)) {
            return FALSE;
        }

        tHalfChordSquared = sphere->radiusSquared - ocSquared +
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
    globalStatistics.raySphereTestsSucceeded++;
    return TRUE;
}

int
Sphere::allSphereIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;
    Intersection localElement;
    int intersectionFound;
    Sphere *shape = (Sphere *)object;

    intersectionFound = FALSE;
    if (Sphere::intersectSphere(ray, shape, &depth1, &depth2)) {
        localElement.Depth = depth1;
        localElement.Object = nullptr;
        VectorOps::vScale(intersectionPoint, ray->direction, depth1);
        intersectionPoint.add(ray->position);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = TRUE;

        if (depth2 != depth1) {
            localElement.Depth = depth2;
            localElement.Object = nullptr;
            VectorOps::vScale(intersectionPoint, ray->direction, depth2);
            intersectionPoint.add(ray->position);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            intersectionFound = TRUE;
        }
    }
    return intersectionFound;
}

int
Sphere::insideSphere(Vector3Dd *testPoint, SimpleBody *object)
{
    Vector3Dd originToCenter;
    double ocSquared;
    Sphere *sphere = (Sphere *)object;

    VectorOps::vSub(originToCenter, sphere->Center, *testPoint);
    ocSquared = originToCenter.dotProduct(originToCenter);

    if (sphere->Inverted) {
        return (ocSquared - sphere->radiusSquared > Small_Tolerance);
    }
    return (ocSquared - sphere->radiusSquared < Small_Tolerance);
}

void
Sphere::sphereNormal(
    Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint)
{
    Sphere *sphere = (Sphere *)object;

    VectorOps::vSub(*result, *intersectionPoint, sphere->Center);
    (*result).scale(sphere->inverseRadius);
}

void *
Sphere::copySphere(SimpleBody *object)
{
    Sphere *newShape;

    newShape = new Sphere;
    *newShape = *((Sphere *)object);
    newShape->nextObject = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture =
            TextureUtils::copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
Sphere::translateSphere(SimpleBody *object, Vector3Dd *vector)
{
    ((Sphere *)object)->Center.add(*vector);
    TextureUtils::translateTexture(&((Sphere *)object)->Shape_Texture, vector);
}

void
Sphere::rotateSphere(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;

    Transformation::getRotationTransformation(&transformation, vector);
    Transformation::MTransformVector(&((Sphere *)object)->Center,
        &((Sphere *)object)->Center, &transformation);
    TextureUtils::rotateTexture(&((Sphere *)object)->Shape_Texture, vector);
}

void
Sphere::scaleSphere(SimpleBody *object, Vector3Dd *vector)
{
    Sphere *sphere = (Sphere *)object;

    if ((vector->x != vector->y) || (vector->x != vector->z)) {
        Logger::error( "Error - you cannot scale a sphere unevenly\n");
        exit(1);
    }

    sphere->Center.scale(vector->x);
    sphere->Radius *= vector->x;
    sphere->radiusSquared = sphere->Radius * sphere->Radius;
    sphere->inverseRadius = 1.0 / sphere->Radius;
    TextureUtils::scaleTexture(&((Sphere *)object)->Shape_Texture, vector);
}

void
Sphere::invertSphere(SimpleBody *object)
{
    ((Sphere *)object)->Inverted ^= TRUE;
}

//===========================================================================
//= EOF =
//===========================================================================
