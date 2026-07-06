#ifndef __POINT_LIGHT__
#define __POINT_LIGHT__

#include "environment/light/Light.h"

class PointLight : public Light {
  public:
    PointLight(const ColorRgba &shapeColor, const Vector3Dd &position);
    PointLight(const PointLight &other);
    double evaluateLightResponseFactor(const Ray *lightSourceRay) const override;
    PointLight *copy() override;
};

inline
PointLight::PointLight(const ColorRgba &shapeColor, const Vector3Dd &position) :
    Light(shapeColor, position)
{
}

inline
PointLight::PointLight(const PointLight &other) :
    Light(other)
{
}

#endif
