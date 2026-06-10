#ifndef __TEXTURE_FIXTURE_H__
#define __TEXTURE_FIXTURE_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"

class ProceduralNoise;

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

  private:
    ProceduralNoise *proceduralNoise;
};

#endif
