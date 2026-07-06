#ifndef __LIGHT__
#define __LIGHT__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"

class Light {
  private:
    ColorRgba shapeColor;
    Vector3Dd position;

  public:
    Light(const ColorRgba &shapeColor, const Vector3Dd &position);
    Light(const Light &other);

    const ColorRgba &getShapeColor() const { return shapeColor; }
    Vector3Dd& getPosition() { return position; }
    const Vector3Dd& getPosition() const { return position; }
    virtual double evaluateLightResponseFactor(const Ray *lightSourceRay) const = 0;
    virtual Light *copy() = 0;
    virtual ~Light() = default;
};

inline
Light::Light(const ColorRgba &shapeColor, const Vector3Dd &position) :
    shapeColor(shapeColor),
    position(position)
{
}

inline
Light::Light(const Light &other) :
    shapeColor(other.shapeColor),
    position(other.position)
{
}

#endif
