#ifndef __POINT_LIGHT__
#define __POINT_LIGHT__

#include "environment/light/Light.h"

class PointLight : public Light {
  public:
    PointLight();
    double evaluateLightResponseFactor(const Ray *lightSourceRay) const override;
    PointLight *copy() override;
};

#endif
