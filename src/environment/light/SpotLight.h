#ifndef __SPOT_LIGHT__
#define __SPOT_LIGHT__

#include "environment/light/Light.h"

class SpotLight : public Light {
  private:
    Vector3Dd pointsAt;
    const double coefficient;
    const double radius;
    const double falloff;

    static double cubicSpline(double low, double high, double pos);

  public:
    SpotLight(const ColorRgba &shapeColor, const Vector3Dd &position,
        const Vector3Dd &pointsAt, double coefficient, double radius,
        double falloff);
    SpotLight(const SpotLight &other);

    Vector3Dd& getPointsAt() { return pointsAt; }
    const Vector3Dd& getPointsAt() const { return pointsAt; }
    double getCoefficient() const { return coefficient; }
    double getRadius() const { return radius; }
    double getFalloff() const { return falloff; }
    double evaluateLightResponseFactor(const Ray *lightSourceRay) const override;
    SpotLight *copy() override;
};

inline
SpotLight::SpotLight(const ColorRgba &shapeColor, const Vector3Dd &position,
    const Vector3Dd &pointsAt, double coefficient, double radius,
    double falloff) :
    Light(shapeColor, position),
    pointsAt(pointsAt),
    coefficient(coefficient),
    radius(radius),
    falloff(falloff)
{
}

inline
SpotLight::SpotLight(const SpotLight &other) :
    Light(other),
    pointsAt(other.pointsAt),
    coefficient(other.coefficient),
    radius(other.radius),
    falloff(other.falloff)
{
}

#endif
