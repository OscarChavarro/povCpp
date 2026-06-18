#ifndef __POINT_LIGHT__
#define __POINT_LIGHT__

#include "environment/light/Light.h"

class PointLight : public Light {
  public:
    PointLight();
    PointLight(const Vector3Dd &center, const Vector3Dd &pointsAt, bool inverted,
        double coefficient, double radius, double falloff);
    double evaluateLightResponseFactor(const Ray *lightSourceRay) const override;
    PointLight *copy() override;
};

#endif
