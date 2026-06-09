#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "common/color/RGBAColor.h"
#include "environment/geometry/GeometryConstants.h"

class Methods;
class Texture;

class Geometry {
  public:
    Methods *methods;
    int Type;
    Geometry *nextObject;
    Texture *Shape_Texture;
    RGBAColor *shapeColor;
};

#endif
