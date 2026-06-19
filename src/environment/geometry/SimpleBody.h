#ifndef __SIMPLE_BODY__
#define __SIMPLE_BODY__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/element/TransformableElement.h"
#include "environment/geometry/Geometry.h"
#include "environment/material/Material.h"

class TextureParser;

class SimpleBody : public TransformableElement {
  private:
    friend class TextureParser;

    Geometry *const geometry = nullptr;
    Material *material = nullptr;
    ColorRgba *shapeColor = nullptr;
    Matrix4x4d transform;
    Matrix4x4d transformInverse;

  public:
    SimpleBody() = default;
    SimpleBody(Geometry *geometry, Material *material, ColorRgba *shapeColor);

    Geometry* getGeometry() const { return geometry; }
    Material* getMaterial() const { return material; }
    ColorRgba* getShapeColor() const { return shapeColor; }
    ColorRgba* ensureShapeColor();
    Matrix4x4d& getTransform() { return transform; }
    const Matrix4x4d& getTransform() const { return transform; }
    Matrix4x4d& getTransformInverse() { return transformInverse; }
    const Matrix4x4d& getTransformInverse() const { return transformInverse; }

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void normal(
        Vector3Dd *result,
        Vector3Dd *intersectionPoint,
        const RenderingConfiguration *config) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;
};

#endif
