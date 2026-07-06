#ifndef __POINT_LIGHT__
#define __POINT_LIGHT__

#include "vsdk/toolkit/environment/light/Light.h"

class PointLight : public Light {
  public:
    PointLight(const Vector3Dd &position, const ColorRgb &emission);
    PointLight(const PointLight &other);

    void getDirectionAndDistance(const Vector3Dd &surfacePoint,
        Vector3Dd *directionOut, double *maxShadowDistanceOut) const override;
    double evaluateLightResponseFactor(const Ray *lightSourceRay) const override;
    Light *copy() const override;
};

#endif
