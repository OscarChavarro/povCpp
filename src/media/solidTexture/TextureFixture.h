#ifndef __TXTTEST_H__
#define __TXTTEST_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"

class TextureFixture {
  public:
    static void painted1(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void painted2(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void painted3(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled);
    static void bumpy1(
        double x, double y, double z, Texture *texture, Vector3Dd *normal,
        int debugEnabled);
    static void bumpy2(
        double x, double y, double z, Texture *texture, Vector3Dd *normal,
        int debugEnabled);
    static void bumpy3(
        double x, double y, double z, Texture *texture, Vector3Dd *normal,
        int debugEnabled);
};

#endif
