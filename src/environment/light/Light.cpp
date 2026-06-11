/**
light.c

This module implements the point & spot light source primitive.
*/

#include "java/lang/Math.h"
#include "environment/light/Light.h"
#include "environment/material/MaterialUtils.h"

Methods Light::methodTable = {
    Light::allPointIntersections, Light::insidePoint, nullptr, Light::copyPoint,
    Light::translatePoint, Light::rotatePoint, Light::scalePoint,
    Light::invertPoint};

int
Light::allPointIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    return (false);
}

int
Light::insidePoint(Vector3Dd *testPoint, SimpleBody *object)
{
    return (false);
}

void *
Light::copyPoint(SimpleBody *object)
{
    Light *newShape = new Light();
    if (newShape == nullptr) {
        return nullptr;
    }
    *newShape = *((Light *)object);
    newShape->nextObject = nullptr;

    if (newShape->material != nullptr) {
        newShape->material =
            MaterialUtils::instance().copyTexture(newShape->material);
    }

    return (newShape);
}

void
Light::translatePoint(SimpleBody *object, Vector3Dd *vector)
{
    ((Light *)object)->Center = ((Light *)object)->Center.add(*vector);
    ((Light *)object)->pointsAt = ((Light *)object)->pointsAt.add(*vector);
}

void
Light::rotatePoint(SimpleBody *object, Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    ((Light *)object)->Center =
        transformation.transpose().multiply(((Light *)object)->Center);
    ((Light *)object)->pointsAt =
        transformation.transpose().multiply(((Light *)object)->pointsAt);
}

void
Light::scalePoint(SimpleBody *object, Vector3Dd *vector)
{
    Matrix4x4d transformation;

    transformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    ((Light *)object)->Center =
        transformation.transpose().multiply(((Light *)object)->Center);
    ((Light *)object)->pointsAt =
        transformation.transpose().multiply(((Light *)object)->pointsAt);
    MaterialUtils::instance().scaleTexture(&((Light *)object)->material, vector);
}

void
Light::invertPoint(SimpleBody *object)
{
    ((Light *)object)->Inverted ^= true;
}

/**
Cubic spline that has tangents of slope 0 at x == low and at x == high.
For a given value "pos" between low and high the spline value is returned
*/
double
Light::cubicSpline(double low, double high, double pos)
{
    // Check to see if the position is within the proper boundaries
    if (pos < low) {
        return 0.0;
    }
    if (pos > high) {
        return 1.0;
    }
    if (high == low) {
        return 0.0;
    }

    // Normalize to the interval 0->1
    pos = (pos - low) / (high - low);

    // See where it is on the cubic curve
    return (3 - 2 * pos) * pos * pos;
}

double
Light::attenuateLight(Light *lightSource, RayWithSegments *lightSourceRay)
{
    double len;
    double costheta;
    double attenuation = 1.0;
    Vector3Dd spotDirection;

    // If this is a spotlight then attenuate based on the incidence angle
    if (lightSource->geometryType == GeometryTypes::SPOT_LIGHT_TYPE) {
        spotDirection =
            lightSource->pointsAt.subtract(lightSource->Center);
        len = spotDirection.length();
        if (len > 0.0) {
            spotDirection = Vector3Dd(spotDirection.x() / len, spotDirection.y() / len, spotDirection.z() / len);
            costheta = lightSourceRay->direction.dotProduct(spotDirection);
            costheta *= -1.0;
            if (costheta > 0.0) {
                attenuation = java::Math::pow(costheta, lightSource->Coeff);
                // If there is a soft falloff region associated with the light
                // then do an interpolation of values between the hot center and
                // the direction at which light falls to nothing.
                if (lightSource->Radius > 0.0) {
                    attenuation *= Light::cubicSpline(
                        lightSource->Falloff, lightSource->Radius, costheta);
                }
                // Logger::info("Atten: %lg\n", Attenuation);
            } else {
                attenuation = 0.0;
            }
        } else {
            attenuation = 0.0;
        }
    }
    return (attenuation);
}
