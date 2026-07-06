#include <cmath>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/light/PointLight.h"

PointLight::PointLight(const Vector3Dd &position, const ColorRgb &emission) :
    Light(position, emission)
{
}

PointLight::PointLight(const PointLight &other) :
    Light(other)
{
}

void
PointLight::getDirectionAndDistance(const Vector3Dd &surfacePoint,
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
PointLight::evaluateLightResponseFactor(const Ray *lightSourceRay) const
{
    (void)lightSourceRay;
    return 1.0;
}

Light *
PointLight::copy() const
{
    return new PointLight(*this);
}
