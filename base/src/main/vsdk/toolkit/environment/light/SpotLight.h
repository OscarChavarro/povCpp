#ifndef __SPOT_LIGHT__
#define __SPOT_LIGHT__

#include "vsdk/toolkit/environment/light/Light.h"

class SpotLight : public Light {
  private:
    Vector3Dd pointsAt;
    double coefficient;
    double radius;
    double falloff;

    static double cubicSpline(double low, double high, double pos);

  public:
    SpotLight(const Vector3Dd &position, const Vector3Dd &pointsAt,
        const ColorRgb &emission, double coefficient, double radius, double falloff);
    SpotLight(const SpotLight &other);

    const Vector3Dd &getPointsAt() const;
    void setPointsAt(const Vector3Dd &pointsAt);
    double getCoefficient() const;
    double getRadius() const;
    double getFalloff() const;

    void getDirectionAndDistance(const Vector3Dd &surfacePoint,
        Vector3Dd *directionOut, double *maxShadowDistanceOut) const override;
    double evaluateLightResponseFactor(const Ray *lightSourceRay) const override;
    Light *copy() const override;
};

#endif
