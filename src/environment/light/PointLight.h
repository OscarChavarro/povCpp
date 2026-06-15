#ifndef __POINT_LIGHT_H__
#define __POINT_LIGHT_H__

#include "environment/light/Light.h"

class PointLight : public Light {
  public:
    PointLight();
    double attenuate(const RayWithSegments *lightSourceRay) const override;
    void *copy() override;
};

#endif
