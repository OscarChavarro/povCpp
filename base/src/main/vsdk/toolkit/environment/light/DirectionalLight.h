#ifndef __DIRECTIONAL_LIGHT__
#define __DIRECTIONAL_LIGHT__

#include "vsdk/toolkit/environment/light/Light.h"

class DirectionalLight : public Light {
  public:
    DirectionalLight(const Vector3Dd &direction, const ColorRgb &emission);
    DirectionalLight(const DirectionalLight &other);

    void getDirectionAndDistance(const Vector3Dd &surfacePoint,
        Vector3Dd *directionOut, double *maxShadowDistanceOut) const override;
    double evaluateLightResponseFactor(const Ray *lightSourceRay) const override;
    Light *copy() const override;
};

#endif
