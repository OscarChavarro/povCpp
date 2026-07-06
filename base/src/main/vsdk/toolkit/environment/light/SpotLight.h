#ifndef __SPOT_LIGHT__
#define __SPOT_LIGHT__

#include "vsdk/toolkit/environment/light/Light.h"

class SpotLight : public Light {
  private:
    Vector3Dd pointsAt;
    const double coefficient;
    const double radius;
    const double falloff;

    static double cubicSpline(double low, double high, double pos);

  public:
    SpotLight(const ColorRgba &color, const Vector3Dd &position,
        const Vector3Dd &pointsAt, double coefficient, double radius,
        double falloff);
    SpotLight(const SpotLight &other);

    const Vector3Dd& getPointsAt() const;
    void setPointsAt(const Vector3Dd &pointsAt);
    double getCoefficient() const;
    double getRadius() const;
    double getFalloff() const;
    double evaluateLightResponseFactor(const Ray *lightSourceRay) const override;
};

inline
SpotLight::SpotLight(const ColorRgba &color, const Vector3Dd &position,
    const Vector3Dd &pointsAt, double coefficient, double radius,
    double falloff) :
    Light(color, position),
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

inline const Vector3Dd&
SpotLight::getPointsAt() const
{
    return pointsAt;
}

inline void
SpotLight::setPointsAt(const Vector3Dd &pointsAt)
{
    this->pointsAt = pointsAt;
}

inline double
SpotLight::getCoefficient() const
{
    return coefficient;
}

inline double
SpotLight::getRadius() const
{
    return radius;
}

inline double
SpotLight::getFalloff() const
{
    return falloff;
}

#endif
