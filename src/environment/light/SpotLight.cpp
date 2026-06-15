/**
light.c

This module implements the spot light source primitive.
*/

#include "java/lang/Math.h"
#include "environment/light/SpotLight.h"
#include "environment/material/MaterialUtils.h"

SpotLight::SpotLight()
{
    this->Center = Vector3Dd(0.0, 0.0, 0.0);
    this->pointsAt = Vector3Dd(0.0, 0.0, 1.0);
    this->geometryType = GeometryTypes::SPOT_LIGHT_TYPE;
    this->Inverted = false;
    this->material = nullptr;
    this->shapeColor = nullptr;
    this->Coeff = 10.0;
    this->Radius = 0.35;
    this->Falloff = 0.35;
    this->Next_Light_Source = nullptr;
}

double
SpotLight::cubicSpline(double low, double high, double pos)
{
    if (pos < low) {
        return 0.0;
    }
    if (pos > high) {
        return 1.0;
    }
    if (high == low) {
        return 0.0;
    }

    pos = (pos - low) / (high - low);
    return (3 - 2 * pos) * pos * pos;
}

double
SpotLight::attenuate(const RayWithSegments *lightSourceRay) const
{
    double len;
    double costheta;
    double attenuation = 1.0;
    Vector3Dd spotDirection;

    spotDirection = this->pointsAt.subtract(this->Center);
    len = spotDirection.length();
    if (len > 0.0) {
        spotDirection = Vector3Dd(spotDirection.x() / len, spotDirection.y() / len, spotDirection.z() / len);
        costheta = lightSourceRay->direction.dotProduct(spotDirection);
        costheta *= -1.0;
        if (costheta > 0.0) {
            attenuation = java::Math::pow(costheta, this->Coeff);
            if (this->Radius > 0.0) {
                attenuation *= SpotLight::cubicSpline(
                    this->Falloff, this->Radius, costheta);
            }
        } else {
            attenuation = 0.0;
        }
    } else {
        attenuation = 0.0;
    }
    return (attenuation);
}

void *
SpotLight::copy()
{
    SpotLight * const newLight = new SpotLight();
    if (newLight == nullptr) {
        return nullptr;
    }
    copyStateInto(newLight);
    return newLight;
}
#include "java/util/PriorityQueue.txx"
