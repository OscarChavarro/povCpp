#ifndef __TXTCOLOR_H__
#define __TXTCOLOR_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "solidTexture/Texture.h"

class ColorTextureFixture {
  public:
    void colorAt(
        ColorRgba *color, Texture *texture, Vector3Dd *intersectionPoint, double smallTolerance);
    void agate(double x, double y, double z, Texture *texture, ColorRgba *color);
    void bozo(double x, double y, double z, Texture *texture, ColorRgba *color);
    void brick(double x, double y, double z, Texture *texture, ColorRgba *color);
    void checker(
        double x, double y, double z, Texture *texture, ColorRgba *color, double smallTolerance);
    void checkerTexture(
        double x, double y, double z, Texture *texture, ColorRgba *color, double smallTolerance);
    void gradient(double x, double y, double z, Texture *texture, ColorRgba *color);
    void granite(double x, double y, double z, Texture *texture, ColorRgba *color);
    void marble(double x, double y, double z, Texture *texture, ColorRgba *color);
    void spotted(double x, double y, double z, Texture *texture, ColorRgba *color);
    void wood(double x, double y, double z, Texture *texture, ColorRgba *color);
    void leopard(double x, double y, double z, Texture *texture, ColorRgba *color);
    void onion(double x, double y, double z, Texture *texture, ColorRgba *color);
};

#endif
