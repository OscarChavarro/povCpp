#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/TransformableElement.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/geometry/elements/GeometryTypes.h"
#include "environment/material/Material.h"

class Geometry : public TransformableElement {
  public:
    GeometryTypes geometryType;
    Geometry *nextObject;
    Material *material;
    ColorRgba *shapeColor;
};

#endif
