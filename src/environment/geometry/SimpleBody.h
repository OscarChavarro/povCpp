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
    SimpleBody *Next_Object;
    Geometry *Bounding_Shapes;
    Geometry *Clipping_Shapes;
    Geometry *Shape;
    char No_Shadow_Flag;
    RGBAColor *Object_Colour;
    Texture *Object_Texture;
};

#endif
