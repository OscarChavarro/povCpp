#ifndef __LIGHT__
#define __LIGHT__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"

class Light {
  private:
    ColorRgba color;
    Vector3Dd position;

  public:
    Light(const ColorRgba &color, const Vector3Dd &position);
    Light(const Light &other);
    virtual ~Light() {}

    const ColorRgba &getColor() const;
    void setColor(const ColorRgba &color);
    const Vector3Dd& getPosition() const;
    void setPosition(const Vector3Dd &position);
    virtual double evaluateLightResponseFactor(const Ray *lightSourceRay) const = 0;
};

inline
Light::Light(const ColorRgba &color, const Vector3Dd &position) :
    color(color),
    position(position)
{
}

inline
Light::Light(const Light &other) :
    color(other.color),
    position(other.position)
{
}

inline const ColorRgba &
Light::getColor() const
{
    return color;
}

inline void
Light::setColor(const ColorRgba &color)
{
    this->color = color;
}

inline const Vector3Dd&
Light::getPosition() const
{
    return position;
}

inline void
Light::setPosition(const Vector3Dd &position)
{
    this->position = position;
}

#endif
