#include "vsdk/toolkit/environment/light/DirectionalLight.h"

DirectionalLight::DirectionalLight(const Vector3Dd &direction, const ColorRgb &emission) :
    Light(direction.normalized(), emission)
{
}

DirectionalLight::DirectionalLight(const DirectionalLight &other) :
    Light(other)
{
}

void
DirectionalLight::getDirectionAndDistance(const Vector3Dd &surfacePoint,
    Vector3Dd *directionOut, double *maxShadowDistanceOut) const
{
    (void)surfacePoint;
    const Vector3Dd &direction = getPosition();
    *directionOut = Vector3Dd(-direction.x(), -direction.y(), -direction.z());
    *maxShadowDistanceOut = 1e308;
}

double
DirectionalLight::evaluateLightResponseFactor(const Ray *lightSourceRay) const
{
    (void)lightSourceRay;
    return 1.0;
}

Light *
DirectionalLight::copy() const
{
    return new DirectionalLight(*this);
}
