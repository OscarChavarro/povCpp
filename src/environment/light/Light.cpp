/**
light.c

This module implements the point & spot light source primitive.
*/

#include "java/lang/Math.h"
#include "environment/light/Light.h"
#include "environment/material/MaterialUtils.h"

int
Light::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    return (false);
}

int
Light::inside(Vector3Dd *point)
{
    return (false);
}

void *
Light::copy()
{
    Light * const newShape = new Light();
    if (newShape == nullptr) {
        return nullptr;
    }
    *newShape = *this;

    if (newShape->material != nullptr) {
        newShape->material =
            MaterialUtils::instance().copyTexture(newShape->material);
    }

    return (newShape);
}

void
Light::translate(Vector3Dd *vector)
{
    this->Center = this->Center.add(*vector);
    this->pointsAt = this->pointsAt.add(*vector);
}

void
Light::rotate(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    this->Center = transformation.transpose().multiply(this->Center);
    this->pointsAt = transformation.transpose().multiply(this->pointsAt);
}

void
Light::scale(Vector3Dd *vector)
{
    Matrix4x4d transformation;

    transformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    this->Center = transformation.transpose().multiply(this->Center);
    this->pointsAt = transformation.transpose().multiply(this->pointsAt);
    MaterialUtils::instance().scaleTexture(&this->material, vector);
}

void
Light::invert()
{
    this->Inverted ^= true;
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
Light::attenuateLight(const Light *lightSource, const RayWithSegments *lightSourceRay)
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
#include "java/util/PriorityQueue.txx"
