#ifndef __LIGHT__
#define __LIGHT__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"

class ParseHelpers;

class Light {
  private:
    friend class ParseHelpers;

    ColorRgba *shapeColor = nullptr;
    Vector3Dd center;
    Vector3Dd pointsAt;
    Light *nextLightSource;
    bool inverted;
    double coefficient;
    double radius;
    double falloff;

  public:
    Light();
    Light(const ColorRgba *shapeColor, const Vector3Dd &center,
        const Vector3Dd &pointsAt, bool inverted, double coefficient,
        double radius, double falloff);
    Light(const Vector3Dd &center, const Vector3Dd &pointsAt, bool inverted,
        double coefficient, double radius, double falloff);

    ColorRgba *getShapeColor() const;
    Vector3Dd& getCenter() { return center; }
    const Vector3Dd& getCenter() const { return center; }
    Vector3Dd& getPointsAt() { return pointsAt; }
    const Vector3Dd& getPointsAt() const { return pointsAt; }
    Light *getNextLightSource() const { return nextLightSource; }
    bool isInverted() const { return inverted; }
    double getCoefficient() const { return coefficient; }
    double getRadius() const { return radius; }
    double getFalloff() const { return falloff; }

    virtual double evaluateLightResponseFactor(const Ray *lightSourceRay) const = 0;

    virtual Light *copy() = 0;
    void applyLinearTransformation(const Matrix4x4d &transformation);
    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void invert();
    virtual ~Light() = default;
};

inline ColorRgba *
Light::getShapeColor() const
{
    return shapeColor;
}

#endif
