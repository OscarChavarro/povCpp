#ifndef __SIMPLE_BODY_H__
#define __SIMPLE_BODY_H__


class Methods;
class Geometry;
class Material;

class SimpleBody {
  public:
    Methods *methods;
    int type;
    SimpleBody *nextObject;
    Geometry *boundingShapes;
    Geometry *clippingShapes;
    Geometry *geometry;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;
};

#endif
