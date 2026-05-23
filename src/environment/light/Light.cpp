/****************************************************************************
 *                     light.c
 *
 *  This module implements the point & spot light source primitive.
 *
 *****************************************************************************/

#include "environment/light/Light.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "io/pov/SceneFactory.h"
#include "io/pov/TextureParser.h"
Methods Point_Methods = {Composite::objectIntersect,
    Light::allPointIntersections, Light::insidePoint, nullptr, Light::copyPoint,
    Light::translatePoint, Light::rotatePoint, Light::scalePoint,
    Light::invertPoint};

int
Light::allPointIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    return (FALSE);
}

int
Light::insidePoint(Vector3Dd *testPoint, SimpleBody *object)
{
    return (FALSE);
}

void *
Light::copyPoint(SimpleBody *object)
{
    Light *newShape;

    newShape = SceneFactory::getLightSourceShape();
    *newShape = *((Light *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture =
            TextureParser::copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
Light::translatePoint(SimpleBody *object, Vector3Dd *vector)
{
    ((Light *)object)->Center.add(*vector);
    ((Light *)object)->Points_At.add(*vector);
}

void
Light::rotatePoint(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;
    Transformation::getRotationTransformation(&transformation, vector);
    Transformation::MTransformVector(&((Light *)object)->Center,
        &((Light *)object)->Center, &transformation);
    Transformation::MTransformVector(&((Light *)object)->Points_At,
        &((Light *)object)->Points_At, &transformation);
}

void
Light::scalePoint(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;
    Transformation::getScalingTransformation(&transformation, vector);
    Transformation::MTransformVector(&((Light *)object)->Center,
        &((Light *)object)->Center, &transformation);
    Transformation::MTransformVector(&((Light *)object)->Points_At,
        &((Light *)object)->Points_At, &transformation);
    TextureUtils::scaleTexture(&((Light *)object)->Shape_Texture, vector);
}

void
Light::invertPoint(SimpleBody *object)
{
    ((Light *)object)->Inverted ^= TRUE;
}

/* Cubic spline that has tangents of slope 0 at x == low and at x == high.
    For a given value "pos" between low and high the spline value is returned */
double
Light::cubicSpline(double low, double high, double pos)
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

double
Light::attenuateLight(Light *lightSource, RayWithSegments *lightSourceRay)
{
    double len, costheta;
    double attenuation = 1.0;
    Vector3Dd spotDirection;

    /* If this is a spotlight then attenuate based on the incidence angle */
    if (lightSource->Type == SPOT_LIGHT_TYPE) {
        VectorOps::vSub(
            spotDirection, lightSource->Points_At, lightSource->Center);
        len = spotDirection.length();
        if (len > 0.0) {
            spotDirection.inverseScale(len);
            costheta = lightSourceRay->direction.dotProduct(spotDirection);
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
