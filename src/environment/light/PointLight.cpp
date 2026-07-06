#include "environment/light/PointLight.h"

double
PointLight::evaluateLightResponseFactor(const Ray *lightSourceRay) const
{
    (void)lightSourceRay;
    return 1.0;
}

PointLight *
PointLight::copy()
{
    return new PointLight(*this);
}
