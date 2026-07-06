#ifndef __POINT_LIGHT__
#define __POINT_LIGHT__

#include "vsdk/toolkit/environment/light/Light.h"

class PointLight : public Light {
  public:
    PointLight(const ColorRgba &color, const Vector3Dd &position);
    PointLight(const PointLight &other);
    double evaluateLightResponseFactor(const Ray *lightSourceRay) const override;
};

inline
PointLight::PointLight(const ColorRgba &color, const Vector3Dd &position) :
    Light(color, position)
{
}

inline
PointLight::PointLight(const PointLight &other) :
    Light(other)
{
}

#endif
