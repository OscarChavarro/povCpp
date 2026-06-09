#ifndef __TXTCOLOR_H__
#define __TXTCOLOR_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"

class ColorTextureFixture {
  public:
    static void colourAt(
        RGBAColor *colour, Texture *texture, Vector3Dd *intersectionPoint, double smallTolerance);
    static void agate(
        double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void bozo(
        double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void brick(
        double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void checker(
        double x, double y, double z, Texture *texture, RGBAColor *colour, double smallTolerance);
    static void checkerTexture(
        double x, double y, double z, Texture *texture, RGBAColor *colour, double smallTolerance);
    static void gradient(
        double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void granite(
        double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void marble(
        double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void spotted(
        double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void wood(
        double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void leopard(
        double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void onion(
        double x, double y, double z, Texture *texture, RGBAColor *colour);
};

#endif
