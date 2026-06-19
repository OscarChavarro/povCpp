#ifndef __SPOT_LIGHT__
#define __SPOT_LIGHT__

#include "environment/light/Light.h"

class SpotLight : public Light {
  public:
    SpotLight();
    SpotLight(const ColorRgba *shapeColor, const Vector3Dd &center,
        const Vector3Dd &pointsAt, bool inverted, double coefficient,
        double radius, double falloff);
    SpotLight(const Vector3Dd &center, const Vector3Dd &pointsAt, bool inverted,
        double coefficient, double radius, double falloff);
    double evaluateLightResponseFactor(const Ray *lightSourceRay) const override;
    SpotLight *copy() override;

  private:
    static double cubicSpline(double low, double high, double pos);
};

#endif
