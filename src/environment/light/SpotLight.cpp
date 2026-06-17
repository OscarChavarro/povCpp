#include "java/lang/Math.h"
#include "environment/light/SpotLight.h"

SpotLight::SpotLight()
{
    this->center = Vector3Dd(0.0, 0.0, 0.0);
    this->pointsAt = Vector3Dd(0.0, 0.0, 1.0);
    this->inverted = false;
    this->coeff = 10.0;
    this->radius = 0.35;
    this->falloff = 0.35;
    this->nextLightSource = nullptr;
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

    spotDirection = this->pointsAt.subtract(this->center);
    len = spotDirection.length();
    if (len > 0.0) {
        spotDirection = Vector3Dd(spotDirection.x() / len, spotDirection.y() / len, spotDirection.z() / len);
        costheta = lightSourceRay->direction.dotProduct(spotDirection);
        costheta *= -1.0;
        if (costheta > 0.0) {
            attenuation = java::Math::pow(costheta, this->coeff);
            if (this->radius > 0.0) {
                attenuation *= SpotLight::cubicSpline(
                    this->falloff, this->radius, costheta);
            }
        } else {
            attenuation = 0.0;
        }
    } else {
        attenuation = 0.0;
    }
    return (attenuation);
}

SpotLight *
SpotLight::copy()
{
    SpotLight * const newLight = new SpotLight();
    if (newLight == nullptr) {
        return nullptr;
    }
    copyStateInto(newLight);
    return newLight;
}
