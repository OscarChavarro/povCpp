#ifndef __SIMPLE_BODY_H__
#define __SIMPLE_BODY_H__


class Methods;
class Geometry;
class Material;

class SimpleBody {
  public:
    Methods *methods;
    int Type;
    SimpleBody *nextObject;
    Geometry *boundingShapes;
    Geometry *clippingShapes;
    Geometry *Shape;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;
};

#endif
