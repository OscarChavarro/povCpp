#include "environment/light/PointLight.h"

PointLight::PointLight() : Light()
{
}

PointLight::PointLight(const ColorRgba *shapeColor, const Vector3Dd &center,
    const Vector3Dd &pointsAt, bool inverted, double coefficient,
    double radius, double falloff) :
    Light(shapeColor, center, pointsAt, inverted, coefficient, radius, falloff)
{
}

PointLight::PointLight(const Vector3Dd &center, const Vector3Dd &pointsAt,
    bool inverted, double coefficient, double radius, double falloff) :
    Light(center, pointsAt, inverted, coefficient, radius, falloff)
{
}

double
PointLight::evaluateLightResponseFactor(const Ray *lightSourceRay) const
{
    (void)lightSourceRay;
    return 1.0;
}

PointLight *
PointLight::copy()
{
    return new PointLight(getShapeColor(), getCenter(), getPointsAt(),
        isInverted(), getCoefficient(), getRadius(), getFalloff());
}
