#ifndef __POINT_H__
#define __POINT_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/material/Material.h"

class Light {
  public:
    Material *material = nullptr;
    ColorRgba *shapeColor = nullptr;
    Vector3Dd center;
    Vector3Dd pointsAt;
    Light *nextLightSource;
    bool inverted;
    double coeff;
    double radius;
    double falloff;

    Material* getMaterial() const { return material; }
    void setMaterial(Material* mat) { material = mat; }
    ColorRgba* getShapeColor() const { return shapeColor; }
    void setShapeColor(ColorRgba* color) { shapeColor = color; }

    virtual double attenuate(const RayWithSegments *lightSourceRay) const = 0;

    virtual Light *copy() = 0;
    void applyLinearTransformation(const Matrix4x4d &transformation);
    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void invert();
    void copyStateInto(Light *dst) const;
    virtual ~Light() = default;
};

#endif
