#ifndef __SIMPLE_BODY_H__
#define __SIMPLE_BODY_H__


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
    bool noShadowFlag;
    RGBAColor *objectColour;
    Texture *objectTexture;
};

#endif
