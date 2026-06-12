#ifndef __TXTCOLOR_H__
#define __TXTCOLOR_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "solidTexture/procedural/ProceduralNoise.h"

class TextureUtils;

class ColorTextureFixture {
  private:
    const ProceduralNoise * const proceduralNoise;
    const TextureUtils * const textureUtils;

  public:
    ColorTextureFixture(const ProceduralNoise *proceduralNoise, const TextureUtils *textureUtils);

    void agate(
        double x, double y, double z, int octaves, const RGBAColorPalette *colorMap,
        ColorRgba *color) const;
    void bozo(
        double x, double y, double z, double turbulence, int octaves,
        const RGBAColorPalette *colorMap, ColorRgba *color) const;
    void brick(
        double x, double y, double z, ColorRgba *color, const ColorRgba *color1,
        const ColorRgba *color2, double mortar) const;
    void checker(
        double x, double y, double z, ColorRgba *color, const ColorRgba *color1,
        const ColorRgba *color2, double smallTolerance) const;
    void gradient(
        double x, double y, double z, double turbulence,
        const RGBAColorPalette *colorMap, Vector3Dd textureGradient, int octaves,
        ColorRgba *color) const;
    void granite(double x, double y, double z, const RGBAColorPalette *colorMap, ColorRgba *color) const;
    void marble(
        double x, double y, double z, double turbulence, int octaves,
        const RGBAColorPalette *colorMap, ColorRgba *color) const;
    void spotted(double x, double y, double z, const RGBAColorPalette *colorMap, ColorRgba *color) const;
    void wood(
        double x, double y, double z, double turbulence, int octaves,
        const RGBAColorPalette *colorMap, ColorRgba *color) const;
    void leopard(
        double x, double y, double z, double turbulence, int octaves,
        const RGBAColorPalette *colorMap, ColorRgba *color) const;
    void onion(
        double x, double y, double z, double turbulence, int octaves,
        const RGBAColorPalette *colorMap, ColorRgba *color) const;
};

#endif
