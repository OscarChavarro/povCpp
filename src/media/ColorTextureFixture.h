#ifndef __TXTCOLOR_H__
#define __TXTCOLOR_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

class ColorTextureFixture {
  public:
    static void colourAt(
        RGBAColor *colour, Texture *texture, Vector3Dd *intersectionPoint,
        int debugEnabled, double smallTolerance);
    static void agate(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void bozo(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void brick(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void checker(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled, double smallTolerance);
    static void checkerTexture(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled, double smallTolerance);
    static void gradient(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void granite(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void marble(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void spotted(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void wood(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void leopard(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void onion(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
};

#endif
