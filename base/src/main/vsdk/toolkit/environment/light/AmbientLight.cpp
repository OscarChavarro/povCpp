#include "vsdk/toolkit/environment/light/AmbientLight.h"

AmbientLight::AmbientLight(const ColorRgb &emission) :
    Light(Vector3Dd(0, 0, 0), emission)
{
}

AmbientLight::AmbientLight(const AmbientLight &other) :
    Light(other)
{
}

bool
AmbientLight::isAmbient() const
{
    return true;
}

void
AmbientLight::getDirectionAndDistance(const Vector3Dd &surfacePoint,
    Vector3Dd *directionOut, double *maxShadowDistanceOut) const
{
    (void)surfacePoint;
    // Never sampled: shaders resolve the ambient contribution from
    // getEmission() directly and skip the shadow ray entirely.
    *directionOut = Vector3Dd(0, 0, 0);
    *maxShadowDistanceOut = 0.0;
}

double
AmbientLight::evaluateLightResponseFactor(const Ray *lightSourceRay) const
{
    (void)lightSourceRay;
    return 0.0;
}

Light *
AmbientLight::copy() const
{
    return new AmbientLight(*this);
}
