#ifndef __TXTCOLOR_H__
#define __TXTCOLOR_H__

#include "common/Frame.h"
#include "common/Vector.h"
#include "media/Texture.h"

class ColorTextures {
  public:
    static void colourAt(
        RGBAColor *colour, Texture *texture, Vector3D *intersectionPoint);
    static void agate(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void bozo(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void brick(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void checker(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void checkerTexture(
        DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void gradient(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void granite(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void marble(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void spotted(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void wood(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void leopard(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void onion(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
};

#endif
