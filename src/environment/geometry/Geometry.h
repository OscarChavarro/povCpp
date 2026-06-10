#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/GeometryConstants.h"

class Methods;
class Material;

class Geometry {
  public:
    Methods *methods;
    int Type;
    Geometry *nextObject;
    Material *Shape_Texture;
    ColorRgba *shapeColor;
};

#endif
