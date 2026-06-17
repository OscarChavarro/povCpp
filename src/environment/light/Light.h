#ifndef __POINT_H__
#define __POINT_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"

class Light {
  public:
    ColorRgba *shapeColor = nullptr;
    Vector3Dd center;
    Vector3Dd pointsAt;
    Light *nextLightSource;
    bool inverted;
    double coefficient;
    double radius;
    double falloff;

    ColorRgba *getShapeColor() const;
    Vector3Dd& getCenter() { return center; }
    const Vector3Dd& getCenter() const { return center; }
    Light *getNextLightSource() const { return nextLightSource; }

    virtual double evaluateLightResponseFactor(const Ray *lightSourceRay) const = 0;

    virtual Light *copy() = 0;
    void applyLinearTransformation(const Matrix4x4d &transformation);
    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void invert();
    void copyStateInto(Light *dst) const;
    virtual ~Light() = default;
};

inline ColorRgba *
Light::getShapeColor() const
{
    return shapeColor;
}

#endif
