#ifndef __SPOT_LIGHT__
#define __SPOT_LIGHT__

#include "environment/light/Light.h"

class SpotLight : public Light {
  public:
    SpotLight();
    double evaluateLightResponseFactor(const Ray *lightSourceRay) const override;
    SpotLight *copy() override;

  private:
    static double cubicSpline(double low, double high, double pos);
};

#endif
