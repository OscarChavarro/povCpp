#ifndef __SIMPLE_BODY_BUILDER__
#define __SIMPLE_BODY_BUILDER__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/TransformedGeometry.h"
#include "environment/material/Material.h"

class SimpleBodyBuilder {
  private:
    Geometry *geometry = nullptr;
    Material *material = nullptr;
    ColorRgba *shapeColor = nullptr;

  public:
    SimpleBodyBuilder() {}
    SimpleBodyBuilder(Geometry *geometry, Material *material, ColorRgba *shapeColor);
    SimpleBodyBuilder(const SimpleBodyBuilder &other);
    ~SimpleBodyBuilder();

    Geometry* getGeometry() const { return geometry; }
    Material* getMaterial() const { return material; }
    ColorRgba* getShapeColor() const { return shapeColor; }
    Geometry* releaseGeometry();
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
