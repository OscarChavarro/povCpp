#ifndef __FIXTURES_FACADE_H__
#define __FIXTURES_FACADE_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "solidTexture/ProceduralNoise.h"
#include "solidTexture/TextureImage.h"
#include "solidTexture/TextureUtils.h"

class SolidTextureFixturesFacade {
  private:
    static constexpr double COORDINATE_LIMIT = 1.0e17;
    ProceduralNoise *proceduralNoise;
    TextureUtils *textureUtils;

  public:
    SolidTextureFixturesFacade(ProceduralNoise *proceduralNoise, TextureUtils *textureUtils);

    void checkerTexture(
        double x, double y, double z, ColorRgba *color,
        int textureNumber1, Matrix4x4d *textureTransformationInverse1,
        TextureImage *image1, ColorRgba *color1_1, ColorRgba *color2_1,
        double turbulence1, int octaves1, RGBAColorPalette *colorMap1,
        Vector3Dd textureGradient1, double mortar1,
        int textureNumber2, Matrix4x4d *textureTransformationInverse2,
        TextureImage *image2, ColorRgba *color1_2, ColorRgba *color2_2,
        double turbulence2, int octaves2, RGBAColorPalette *colorMap2,
        Vector3Dd textureGradient2, double mortar2,
        double smallTolerance);

    void colorAt(
        ColorRgba *color, int textureNumber,
        Matrix4x4d *textureTransformationInverse, TextureImage *image,
        ColorRgba *color1, ColorRgba *color2, double turbulence, int octaves,
        RGBAColorPalette *colorMap, Vector3Dd textureGradient, double mortar,
        Vector3Dd *intersectionPoint, double smallTolerance,
        int textureNumber1 = 0, Matrix4x4d *textureTransformationInverse1 = nullptr,
        TextureImage *image1 = nullptr, ColorRgba *color1_1 = nullptr,
        ColorRgba *color2_1 = nullptr, double turbulence1 = 0.0, int octaves1 = 0,
        RGBAColorPalette *colorMap1 = nullptr, Vector3Dd textureGradient1 = Vector3Dd(),
        double mortar1 = 0.0,
        int textureNumber2 = 0, Matrix4x4d *textureTransformationInverse2 = nullptr,
        TextureImage *image2 = nullptr, ColorRgba *color1_2 = nullptr,
        ColorRgba *color2_2 = nullptr, double turbulence2 = 0.0, int octaves2 = 0,
        RGBAColorPalette *colorMap2 = nullptr, Vector3Dd textureGradient2 = Vector3Dd(),
        double mortar2 = 0.0);
};

#endif
