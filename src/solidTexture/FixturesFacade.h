#ifndef __FIXTURES_FACADE_H__
#define __FIXTURES_FACADE_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "solidTexture/ProceduralNoise.h"
#include "solidTexture/TextureImage.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"

class TextureUtils;

class FixturesFacade {
  private:
    static constexpr double COORDINATE_LIMIT = 1.0e17;
    ProceduralNoise *proceduralNoise;
    TextureUtils *textureUtils;

  public:
    FixturesFacade(ProceduralNoise *proceduralNoise, TextureUtils *textureUtils);

    void checkerTexture(
        double x, double y, double z, ColorRgba *color, ColorRgba *color1,
        ColorRgba *color2, double smallTolerance);

    void colorAt(
        ColorRgba *color, int textureNumber,
        Matrix4x4d *textureTransformationInverse, TextureImage *image,
        ColorRgba *color1, ColorRgba *color2, double turbulence, int octaves,
        RGBAColorPalette *colorMap, Vector3Dd textureGradient, double mortar,
        Vector3Dd *intersectionPoint, double smallTolerance);
};

#endif
