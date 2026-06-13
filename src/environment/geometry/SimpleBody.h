#ifndef __SIMPLE_BODY_H__
#define __SIMPLE_BODY_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/elements/GeometryTypes.h"
#include "environment/material/Material.h"

class Methods;

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
