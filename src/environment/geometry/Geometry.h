#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "common/FrameConfig.h"

class Methods;
class Texture;

class Geometry {
  public:
    Methods *methods;
    int Type;
    Geometry *Next_Object;
    Texture *Shape_Texture;
    RGBAColor *Shape_Colour;
};

#endif
