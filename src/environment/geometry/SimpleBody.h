#ifndef __SIMPLE_BODY_H__
#define __SIMPLE_BODY_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/GeometryTypes.h"

class Methods;
class Geometry;
class Material;

class SimpleBody {
  public:
    Methods *methods;
    GeometryTypes type;
    SimpleBody *nextObject;
    Geometry *boundingShapes;
    Geometry *clippingShapes;
    Geometry *geometry;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;
};

#endif
