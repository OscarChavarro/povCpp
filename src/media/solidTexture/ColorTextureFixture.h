#ifndef __TXTCOLOR_H__
#define __TXTCOLOR_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"

class colorTextureFixture {
  public:
    void colorAt(
        RGBAColor *color, Texture *texture, Vector3Dd *intersectionPoint, double smallTolerance);
    void agate(double x, double y, double z, Texture *texture, RGBAColor *color);
    void bozo(double x, double y, double z, Texture *texture, RGBAColor *color);
    void brick(double x, double y, double z, Texture *texture, RGBAColor *color);
    void checker(
        double x, double y, double z, Texture *texture, RGBAColor *color, double smallTolerance);
    void checkerTexture(
        double x, double y, double z, Texture *texture, RGBAColor *color, double smallTolerance);
    void gradient(double x, double y, double z, Texture *texture, RGBAColor *color);
    void granite(double x, double y, double z, Texture *texture, RGBAColor *color);
    void marble(double x, double y, double z, Texture *texture, RGBAColor *color);
    void spotted(double x, double y, double z, Texture *texture, RGBAColor *color);
    void wood(double x, double y, double z, Texture *texture, RGBAColor *color);
    void leopard(double x, double y, double z, Texture *texture, RGBAColor *color);
    void onion(double x, double y, double z, Texture *texture, RGBAColor *color);
};

#endif
