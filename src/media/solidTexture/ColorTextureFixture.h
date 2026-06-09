#ifndef __TXTCOLOR_H__
#define __TXTCOLOR_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"

class colorTextureFixture {
  public:
    void colorAt(
        RGBAColor *colour, Texture *texture, Vector3Dd *intersectionPoint, double smallTolerance);
    void agate(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void bozo(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void brick(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void checker(
        double x, double y, double z, Texture *texture, RGBAColor *colour, double smallTolerance);
    void checkerTexture(
        double x, double y, double z, Texture *texture, RGBAColor *colour, double smallTolerance);
    void gradient(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void granite(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void marble(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void spotted(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void wood(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void leopard(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void onion(double x, double y, double z, Texture *texture, RGBAColor *colour);
};

#endif
