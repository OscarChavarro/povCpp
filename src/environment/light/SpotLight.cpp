#include "java/lang/Math.h"
#include "environment/light/SpotLight.h"

SpotLight::SpotLight() : Light()
{
}

SpotLight::SpotLight(const Vector3Dd &center, const Vector3Dd &pointsAt,
    bool inverted, double coefficient, double radius, double falloff) :
    Light(center, pointsAt, inverted, coefficient, radius, falloff)
{
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
SpotLight::evaluateLightResponseFactor(const Ray *lightSourceRay) const
{
    double attenuation;
    Vector3Dd spotDirection = this->getPointsAt().subtract(this->getCenter());

    double len = spotDirection.length();
    if (len > 0.0) {
        spotDirection = Vector3Dd(spotDirection.x() / len, spotDirection.y() / len, spotDirection.z() / len);
        double cosTheta = lightSourceRay->getDirection().dotProduct(spotDirection);
        cosTheta *= -1.0;
        if (cosTheta > 0.0) {
            attenuation = java::Math::pow(cosTheta, this->getCoefficient());
            if (this->getRadius() > 0.0) {
                attenuation *= SpotLight::cubicSpline(
                    this->getFalloff(), this->getRadius(), cosTheta);
            }
        } else {
            attenuation = 0.0;
        }
    } else {
        attenuation = 0.0;
    }
    return attenuation;
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
