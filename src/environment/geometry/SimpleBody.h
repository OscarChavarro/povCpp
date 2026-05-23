#ifndef __SIMPLE_BODY_H__
#define __SIMPLE_BODY_H__

#include "common/LegacyBoolean.h"

class Methods;
class Geometry;
class Texture;

class SimpleBody {
  public:
    Methods *methods;
    int Type;
    SimpleBody *nextObject;
    Geometry *boundingShapes;
    Geometry *clippingShapes;
    Geometry *Shape;
    char noShadowFlag;
    RGBAColor *objectColour;
    Texture *objectTexture;
};

#endif
