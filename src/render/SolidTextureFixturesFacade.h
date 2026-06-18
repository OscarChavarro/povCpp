#ifndef __SOLID_TEXTURE_FIXTURES_FACADE__
#define __SOLID_TEXTURE_FIXTURES_FACADE__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/media/solidTexture/procedural/ProceduralNoise.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"

class SolidTextureFixturesFacade {
  private:
    static constexpr double COORDINATE_LIMIT = 1.0e17;
    const ProceduralNoise * const proceduralNoise;
    const TextureUtils * const textureUtils;

  public:
    SolidTextureFixturesFacade(const ProceduralNoise *proceduralNoise, const TextureUtils *textureUtils);

    void checkerTexture(
        double x, double y, double z, ColorRgba *color,
        int textureNumber1, const Matrix4x4d *textureTransformationInverse1,
        const ControlledRGBAImageHDRUncompressed *image1, const ColorRgba *color1_1, const ColorRgba *color2_1,
        double turbulence1, int octaves1, const RGBAColorPalette *colorMap1,
        Vector3Dd textureGradient1, double mortar1,
        int textureNumber2, const Matrix4x4d *textureTransformationInverse2,
        const ControlledRGBAImageHDRUncompressed *image2, const ColorRgba *color1_2, const ColorRgba *color2_2,
        double turbulence2, int octaves2, const RGBAColorPalette *colorMap2,
        Vector3Dd textureGradient2, double mortar2,
        double smallTolerance) const;

    void colorAt(
        ColorRgba *color, int textureNumber,
        const Matrix4x4d *textureTransformationInverse, const ControlledRGBAImageHDRUncompressed *image,
        const ColorRgba *color1, const ColorRgba *color2, double turbulence, int octaves,
        const RGBAColorPalette *colorMap, Vector3Dd textureGradient, double mortar,
        const Vector3Dd *intersectionPoint, double smallTolerance,
        int textureNumber1 = 0, const Matrix4x4d *textureTransformationInverse1 = nullptr,
        const ControlledRGBAImageHDRUncompressed *image1 = nullptr, const ColorRgba *color1_1 = nullptr,
        const ColorRgba *color2_1 = nullptr, double turbulence1 = 0.0, int octaves1 = 0,
        const RGBAColorPalette *colorMap1 = nullptr, Vector3Dd textureGradient1 = Vector3Dd(),
        double mortar1 = 0.0,
        int textureNumber2 = 0, const Matrix4x4d *textureTransformationInverse2 = nullptr,
        const ControlledRGBAImageHDRUncompressed *image2 = nullptr, const ColorRgba *color1_2 = nullptr,
        const ColorRgba *color2_2 = nullptr, double turbulence2 = 0.0, int octaves2 = 0,
        const RGBAColorPalette *colorMap2 = nullptr, Vector3Dd textureGradient2 = Vector3Dd(),
        double mortar2 = 0.0) const;
};

#endif
