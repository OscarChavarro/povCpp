#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/TransformableElement.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/material/Material.h"

class Geometry : public TransformableElement {
  public:
    Material *material;
    ColorRgba *shapeColor;

    Material* getMaterial() const { return material; }
    void setMaterial(Material* mat) { material = mat; }
    ColorRgba* getShapeColor() const { return shapeColor; }
    void setShapeColor(ColorRgba* color) { shapeColor = color; }
};

#endif
