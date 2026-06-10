#ifndef __TXTCOLOR_H__
#define __TXTCOLOR_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "solidTexture/ProceduralNoise.h"
#include "solidTexture/Texture.h"

class TextureUtils;

class ColorTextureFixture {
  private:
    ProceduralNoise *proceduralNoise;
    TextureUtils *textureUtils;

  public:
    ColorTextureFixture(ProceduralNoise *proceduralNoise, TextureUtils *textureUtils);
    void agate(
        double x, double y, double z, int octaves, RGBAColorPalette *colorMap,
        ColorRgba *color);
    void bozo(
        double x, double y, double z, double turbulence, int octaves,
        RGBAColorPalette *colorMap, ColorRgba *color);
    void brick(
        double x, double y, double z, ColorRgba *color, ColorRgba *color1,
        ColorRgba *color2, double mortar);
    void checker(
        double x, double y, double z, ColorRgba *color, ColorRgba *color1,
        ColorRgba *color2, double smallTolerance);
    void gradient(
        double x, double y, double z, double turbulence,
        RGBAColorPalette *colorMap, Vector3Dd textureGradient, int octaves,
        ColorRgba *color);
    void granite(double x, double y, double z, RGBAColorPalette *colorMap, ColorRgba *color);
    void marble(
        double x, double y, double z, double turbulence, int octaves,
        RGBAColorPalette *colorMap, ColorRgba *color);
    void spotted(double x, double y, double z, RGBAColorPalette *colorMap, ColorRgba *color);
    void wood(
        double x, double y, double z, double turbulence, int octaves,
        RGBAColorPalette *colorMap, ColorRgba *color);
    void leopard(
        double x, double y, double z, double turbulence, int octaves,
        RGBAColorPalette *colorMap, ColorRgba *color);
    void onion(
        double x, double y, double z, double turbulence, int octaves,
        RGBAColorPalette *colorMap, ColorRgba *color);
};

#endif
