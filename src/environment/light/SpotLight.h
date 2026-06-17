#ifndef __SPOT_LIGHT_H__
#define __SPOT_LIGHT_H__

#include "environment/light/Light.h"

class SpotLight : public Light {
  public:
    SpotLight();
    double attenuate(const RayWithSegments *lightSourceRay) const override;
    SpotLight *copy() override;

  private:
    static double cubicSpline(double low, double high, double pos);
};

#endif
