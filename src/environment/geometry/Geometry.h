#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "common/FrameConfig.h"

class Methods;
class SimpleBody;
class Texture;

class Geometry {
  public:
    Methods *methods;
    int Type;
    Geometry *Next_Object;
    SimpleBody *Parent_Object;
    Texture *Shape_Texture;
    RGBAColor *Shape_Colour;
};

#endif
