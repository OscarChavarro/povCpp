#ifndef __TXTTEST_H__
#define __TXTTEST_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "solidTexture/ProceduralNoise.h"
#include "solidTexture/Texture.h"

class TextureFixture {
  public:
    TextureFixture(ProceduralNoise *proceduralNoise);

    void painted1(
        double x, double y, double z, RGBAColorPalette *colorMap,
        ColorRgba *color);
    void painted2(
        double x, double y, double z, double turbulence, int octaves,
        RGBAColorPalette *colorMap, ColorRgba *color);
    void painted3(double x, double y, double z, ColorRgba *color);
    void bumpy1(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void bumpy2(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void bumpy3(double x, double y, double z, Texture *texture, Vector3Dd *normal);

  private:
    ProceduralNoise *proceduralNoise;
};

#endif
