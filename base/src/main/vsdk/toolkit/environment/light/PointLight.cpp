#include "vsdk/toolkit/environment/light/PointLight.h"

double
PointLight::evaluateLightResponseFactor(const Ray *lightSourceRay) const
{
    (void)lightSourceRay;
    return 1.0;
}
