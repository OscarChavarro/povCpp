#ifndef __SIMPLE_BODY__
#define __SIMPLE_BODY__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/element/TransformableElement.h"
#include "environment/geometry/Geometry.h"
#include "environment/material/Material.h"

class SimpleBody : public TransformableElement {
  private:
    Geometry *const geometry = nullptr;
    Material *material = nullptr;
    ColorRgba *shapeColor = nullptr;

  public:
    SimpleBody() {}
    SimpleBody(Geometry *geometry, Material *material, ColorRgba *shapeColor);
    SimpleBody(const SimpleBody &other);
    ~SimpleBody() override;

    Geometry* getGeometry() const { return geometry; }
    Material* getMaterial() const { return material; }
    ColorRgba* getShapeColor() const { return shapeColor; }
    ColorRgba* ensureShapeColor();
    void prependMaterialLayers(Material *newHead);

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
