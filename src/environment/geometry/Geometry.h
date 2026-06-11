#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/geometry/GeometryTypes.h"

class Methods;
class Material;

class Geometry {
  public:
    Methods *methods;
    GeometryTypes geometryType;
    Geometry *nextObject;
    Material *material;
    ColorRgba *shapeColor;
};

#endif
