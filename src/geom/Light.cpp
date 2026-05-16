/****************************************************************************
 *                     light.c
 *
 *  This module implements the point & spot light source primitive.
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

#include "geom/Light.h"
#include "geom/Objects.h"

/*===========================================================================*/

Methods Point_Methods = {Object_Intersect, All_Point_Intersections,
    Inside_Point, nullptr, Copy_Point, Translate_Point, Rotate_Point,
    Scale_Point, Invert_Point};

extern Light *Get_Light_Source_Shape();

/*===========================================================================*/

int
All_Point_Intersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    return (FALSE);
}

int
Inside_Point(Vector3D *testPoint, SimpleBody *object)
{
    return (FALSE);
}

void *
Copy_Point(SimpleBody *object)
{
    Light *newShape;

    newShape = Get_Light_Source_Shape();
    *newShape = *((Light *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = Copy_Texture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
Translate_Point(SimpleBody *object, Vector3D *vector)
{
    VAdd(((Light *)object)->Center, ((Light *)object)->Center, *vector);
    VAdd(((Light *)object)->Points_At, ((Light *)object)->Points_At, *vector);
}

void
Rotate_Point(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;
    Get_Rotation_Transformation(&transformation, vector);
    MTransformVector(&((Light *)object)->Center, &((Light *)object)->Center,
        &transformation);
    MTransformVector(&((Light *)object)->Points_At,
        &((Light *)object)->Points_At, &transformation);
}

void
Scale_Point(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;
    Get_Scaling_Transformation(&transformation, vector);
    MTransformVector(&((Light *)object)->Center, &((Light *)object)->Center,
        &transformation);
    MTransformVector(&((Light *)object)->Points_At,
        &((Light *)object)->Points_At, &transformation);
    Scale_Texture(&((Light *)object)->Shape_Texture, vector);
}

void
Invert_Point(SimpleBody *object)
{
    ((Light *)object)->Inverted ^= TRUE;
}

/* Cubic spline that has tangents of slope 0 at x == low and at x == high.
    For a given value "pos" between low and high the spline value is returned */
static DBL
cubicSpline(DBL low, DBL high, DBL pos)
{
    /* Check to see if the position is within the proper boundaries */
    if (pos < low) {
        return 0.0;
    }
    if (pos > high) {
        return 1.0;
    }
    if (high == low) {
        return 0.0;
    }

    /* Normalize to the interval 0->1 */
    pos = (pos - low) / (high - low);

    /* See where it is on the cubic curve */
    return (3 - 2 * pos) * pos * pos;
}

DBL
Attenuate_Light(Light *lightSource, Ray *lightSourceRay)
{
    DBL len, costheta;
    DBL attenuation = 1.0;
    Vector3D spotDirection;

    /* If this is a spotlight then attenuate based on the incidence angle */
    if (lightSource->Type == SPOT_LIGHT_TYPE) {
        VSub(spotDirection, lightSource->Points_At, lightSource->Center);
        VLength(len, spotDirection);
        if (len > 0.0) {
            VInverseScale(spotDirection, spotDirection, len);
            VDot(costheta, lightSourceRay->Direction, spotDirection);
            costheta *= -1.0;
            if (costheta > 0.0) {
                attenuation = pow(costheta, lightSource->Coeff);
                /* If there is a soft falloff region associated with the light
                   then do an interpolation of values between the hot center and
                   the direction at which light falls to nothing. */
                if (lightSource->Radius > 0.0) {
                    attenuation *= cubicSpline(
                        lightSource->Falloff, lightSource->Radius, costheta);
                }
                /* printf("Atten: %lg\n", Attenuation); */
            } else {
                attenuation = 0.0;
            }
        } else {
            attenuation = 0.0;
        }
    }
    return (attenuation);
}
