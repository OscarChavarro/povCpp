/****************************************************************************
 *                     light.c
 *
 *  This module implements the point & spot light source primitive.
 *
 *****************************************************************************/

#include "geom/Light.h"
#include "geom/Objects.h"
Methods Point_Methods = {objectIntersect, Light::allPointIntersections,
    Light::insidePoint, nullptr, Light::copyPoint, Light::translatePoint, Light::rotatePoint,
    Light::scalePoint, Light::invertPoint};

extern Light *getLightSourceShape();
int
Light::allPointIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    return (FALSE);
}

int
Light::insidePoint(Vector3D *testPoint, SimpleBody *object)
{
    return (FALSE);
}

void *
Light::copyPoint(SimpleBody *object)
{
    Light *newShape;

    newShape = getLightSourceShape();
    *newShape = *((Light *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
Light::translatePoint(SimpleBody *object, Vector3D *vector)
{
    VAdd(((Light *)object)->Center, ((Light *)object)->Center, *vector);
    VAdd(((Light *)object)->Points_At, ((Light *)object)->Points_At, *vector);
}

void
Light::rotatePoint(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;
    Transformation::getRotationTransformation(&transformation, vector);
    Transformation::MTransformVector(&((Light *)object)->Center, &((Light *)object)->Center,
        &transformation);
    Transformation::MTransformVector(&((Light *)object)->Points_At,
        &((Light *)object)->Points_At, &transformation);
}

void
Light::scalePoint(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;
    Transformation::getScalingTransformation(&transformation, vector);
    Transformation::MTransformVector(&((Light *)object)->Center, &((Light *)object)->Center,
        &transformation);
    Transformation::MTransformVector(&((Light *)object)->Points_At,
        &((Light *)object)->Points_At, &transformation);
    scaleTexture(&((Light *)object)->Shape_Texture, vector);
}

void
Light::invertPoint(SimpleBody *object)
{
    ((Light *)object)->Inverted ^= TRUE;
}

/* Cubic spline that has tangents of slope 0 at x == low and at x == high.
    For a given value "pos" between low and high the spline value is returned */
DBL
Light::cubicSpline(DBL low, DBL high, DBL pos)
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
Light::attenuateLight(Light *lightSource, Ray *lightSourceRay)
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
                    attenuation *= Light::cubicSpline(
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
