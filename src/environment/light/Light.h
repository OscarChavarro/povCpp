#ifndef __POINT_H__
#define __POINT_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/elements/GeometryTypes.h"
#include "environment/material/PovrayMaterial.h"

// A Light is reached directly as a Light* (lightSources list / LightSamplerShader)
// and is never shaded through Intersection::Shape, so unlike other Geometry it
// keeps owning its own emission colour and material.
class Light : public Geometry {
  public:
    GeometryTypes geometryType;
    PovrayMaterial *material = nullptr;
    ColorRgba *shapeColor = nullptr;
    Vector3Dd center;
    Vector3Dd pointsAt;
    Light *nextLightSource;
    bool inverted;
    double coeff;
    double radius;
    double falloff;

    PovrayMaterial* getMaterial() const { return material; }
    void setMaterial(PovrayMaterial* mat) { material = mat; }
    ColorRgba* getShapeColor() const { return shapeColor; }
    void setShapeColor(ColorRgba* color) { shapeColor = color; }

    virtual double attenuate(const RayWithSegments *lightSourceRay) const = 0;

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int allIntersectionsForOwner(
        RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue,
        SimpleBody *owner) override;
    int inside(Vector3Dd *point) override;
    void *copy() override = 0;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;
    void copyStateInto(Light *dst) const;
};

#endif
