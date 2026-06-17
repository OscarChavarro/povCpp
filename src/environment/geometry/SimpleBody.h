#ifndef __SIMPLE_BODY_H__
#define __SIMPLE_BODY_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/element/TransformableElement.h"
#include "environment/geometry/Geometry.h"
#include "environment/material/Material.h"

class SimpleBody : public TransformableElement {
  public:
    Geometry *geometry = nullptr;
    Material *material = nullptr;
    ColorRgba *shapeColor = nullptr;
    Matrix4x4d transform;
    Matrix4x4d transformInverse;

    Material* getMaterial() const { return material; }
    void setMaterial(Material* mat) { material = mat; }
    ColorRgba* getShapeColor() const { return shapeColor; }
    void setShapeColor(ColorRgba* color) { shapeColor = color; }
    Matrix4x4d& getTransform() { return transform; }
    Matrix4x4d& getTransformInverse() { return transformInverse; }

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;
};

#endif
