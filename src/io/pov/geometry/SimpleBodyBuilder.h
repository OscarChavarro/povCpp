#ifndef __SIMPLE_BODY_BUILDER__
#define __SIMPLE_BODY_BUILDER__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/TransformedGeometry.h"
#include "environment/material/Material.h"

class SimpleBodyBuilder {
  private:
    TransformedGeometry *geometry = nullptr;
    Material *material = nullptr;
    ColorRgba *shapeColor = nullptr;

  public:
    SimpleBodyBuilder() {}
    SimpleBodyBuilder(TransformedGeometry *geometry, Material *material, ColorRgba *shapeColor);
    SimpleBodyBuilder(const SimpleBodyBuilder &other);
    ~SimpleBodyBuilder();

    TransformedGeometry* getGeometry() const { return geometry; }
    Material* getMaterial() const { return material; }
    ColorRgba* getShapeColor() const { return shapeColor; }
    TransformedGeometry* releaseGeometry();
    Material* releaseMaterial();
    ColorRgba* releaseShapeColor();
    ColorRgba* ensureShapeColor();
    void prependMaterialLayers(Material *newHead);

    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void invert();
};

#endif
