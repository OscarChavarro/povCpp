#include <cmath>

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/light/SpotLight.h"

SpotLight::SpotLight(const Vector3Dd &position, const Vector3Dd &pointsAt,
    const ColorRgb &emission, double coefficient, double radius, double falloff) :
    Light(position, emission),
    pointsAt(pointsAt),
    coefficient(coefficient),
    radius(radius),
    falloff(falloff)
{
}

SpotLight::SpotLight(const SpotLight &other) :
    Light(other),
    pointsAt(other.pointsAt),
    coefficient(other.coefficient),
    radius(other.radius),
    falloff(other.falloff)
{
}

const Vector3Dd &
SpotLight::getPointsAt() const
{
    return pointsAt;
}

void
SpotLight::setPointsAt(const Vector3Dd &newPointsAt)
{
    pointsAt = Vector3Dd::copyOf(newPointsAt);
}

double
SpotLight::getCoefficient() const
{
    return coefficient;
}

double
SpotLight::getRadius() const
{
    return radius;
}

double
SpotLight::getFalloff() const
{
    return falloff;
}

void
SpotLight::getDirectionAndDistance(const Vector3Dd &surfacePoint,
    Vector3Dd *directionOut, double *maxShadowDistanceOut) const
{
    Vector3Dd toLight = getPosition().subtract(surfacePoint);
    double distance = toLight.length();
    if ( distance <= VSDK::EPSILON ) {
        *directionOut = Vector3Dd(0, 0, 0);
        *maxShadowDistanceOut = 0.0;
        return;
    }
    *directionOut = Vector3Dd(toLight.x() / distance, toLight.y() / distance, toLight.z() / distance);
    *maxShadowDistanceOut = distance - VSDK::EPSILON;
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
    Vector3Dd spotDirection = getPointsAt().subtract(getPosition());

    double len = spotDirection.length();
    if ( len > 0.0 ) {
        spotDirection = Vector3Dd(spotDirection.x() / len, spotDirection.y() / len, spotDirection.z() / len);
        double cosTheta = lightSourceRay->getDirection().dotProduct(spotDirection);
        cosTheta *= -1.0;
        if ( cosTheta > 0.0 ) {
            attenuation = java::Math::pow(cosTheta, getCoefficient());
            if ( getRadius() > 0.0 ) {
                attenuation *= SpotLight::cubicSpline(getFalloff(), getRadius(), cosTheta);
            }
        } else {
            attenuation = 0.0;
        }
    } else {
        attenuation = 0.0;
    }
    return attenuation;
}

Light *
SpotLight::copy() const
{
    return new SpotLight(*this);
}
