/****************************************************************************
 *                     spheres.c
 *
 *  This module implements the sphere primitive.
 * *****************************************************************************/

#include "environment/geometry/volume/Sphere.h"
#include <cmath>
#include "common/Statistics.h"
#include "media/solidTexture/Texture.h"

//===========================================================================

Methods Sphere::methodTable = {
    Sphere::allSphereIntersections, Sphere::insideSphere, Sphere::sphereNormal,
    Sphere::copySphere, Sphere::translateSphere, Sphere::rotateSphere,
    Sphere::scaleSphere, Sphere::invertSphere};


//===========================================================================

/**
Study closely this method!
*/
int
Sphere::intersectSphere(
    RayWithSegments *ray, Sphere *sphere, double *depth1, double *depth2)
{
    Statistics::global().raySphereTests++;

    //--------------------------------------------------------------------------
    Vector3Dd originToCenter;
    double ocSquared;
    double tClosestApproach;
    double halfChord;
    double tHalfChordSquared;
    short inside;

    if (ray->isPrimaryRay) {
        if (!sphere->VPCached) {
            sphere->VPOtoC = sphere->Center.subtract(ray->position);
            sphere->VPOCSquared = sphere->VPOtoC.dotProduct(sphere->VPOtoC);
            sphere->VPinside = (sphere->VPOCSquared < sphere->radiusSquared);
            sphere->VPCached = true;
        }
        tClosestApproach = sphere->VPOtoC.dotProduct(ray->direction);
        if (!sphere->VPinside && (tClosestApproach < GeometryConstants::Small_Tolerance)) {
            return false;
        }
        tHalfChordSquared = sphere->radiusSquared - sphere->VPOCSquared +
                            (tClosestApproach * tClosestApproach);
    } else {
        originToCenter = sphere->Center.subtract(ray->position);
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

    halfChord = sqrt(tHalfChordSquared);
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

    //--------------------------------------------------------------------------
    Statistics::global().raySphereTestsSucceeded++;
    return true;
}

int
Sphere::allSphereIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;
    Intersection localElement;
    bool intersectionFound;
    Sphere *shape = (Sphere *)object;

    intersectionFound = false;
    if (Sphere::intersectSphere(ray, shape, &depth1, &depth2)) {
        localElement.Depth = depth1;
        localElement.Object = nullptr;
        intersectionPoint = ray->direction.multiply(depth1);
        intersectionPoint = intersectionPoint.add(ray->position);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = true;

        if (depth2 != depth1) {
            localElement.Depth = depth2;
            localElement.Object = nullptr;
            intersectionPoint = ray->direction.multiply(depth2);
            intersectionPoint = intersectionPoint.add(ray->position);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            intersectionFound = true;
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

    originToCenter = sphere->Center.subtract(*testPoint);
    ocSquared = originToCenter.dotProduct(originToCenter);

    if (sphere->Inverted) {
        return (ocSquared - sphere->radiusSquared > GeometryConstants::Small_Tolerance);
    }
    return (ocSquared - sphere->radiusSquared < GeometryConstants::Small_Tolerance);
}

void
Sphere::sphereNormal(
    Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint)
{
    Sphere *sphere = (Sphere *)object;

    *result = intersectionPoint->subtract(sphere->Center);
    *result = (*result).multiply(sphere->inverseRadius);
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
            TextureUtils::instance().copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
Sphere::translateSphere(SimpleBody *object, Vector3Dd *vector)
{
    ((Sphere *)object)->Center = ((Sphere *)object)->Center.add(*vector);
    TextureUtils::instance().translateTexture(&((Sphere *)object)->Shape_Texture, vector);
}

void
Sphere::rotateSphere(SimpleBody *object, Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    ((Sphere *)object)->Center =
        transformation.transpose().multiply(((Sphere *)object)->Center);
    TextureUtils::instance().rotateTexture(&((Sphere *)object)->Shape_Texture, vector);
}

void
Sphere::scaleSphere(SimpleBody *object, Vector3Dd *vector)
{
    Sphere *sphere = (Sphere *)object;

    if ((vector->x() != vector->y()) || (vector->x() != vector->z())) {
        const double s = (std::fabs(vector->x()) + std::fabs(vector->y()) + std::fabs(vector->z())) / 3.0;
        *vector = Vector3Dd(s, s, s);
    }

    sphere->Center = sphere->Center.multiply(vector->x());
    sphere->Radius *= vector->x();
    sphere->radiusSquared = sphere->Radius * sphere->Radius;
    sphere->inverseRadius = 1.0 / sphere->Radius;
    TextureUtils::instance().scaleTexture(&((Sphere *)object)->Shape_Texture, vector);
}

void
Sphere::invertSphere(SimpleBody *object)
{
    ((Sphere *)object)->Inverted ^= true;
}

//===========================================================================
//= EOF =
//===========================================================================
