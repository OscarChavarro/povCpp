#ifndef __SOLID_TEXTURE_FIXTURES_FACADE__
#define __SOLID_TEXTURE_FIXTURES_FACADE__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/media/solidTexture/procedural/ProceduralNoise.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"

#include "render/SolidTextureFixturesColorAtParameterSet.h"
#include "render/SolidTextureFixturesCheckerParameterSet.h"

class SolidTextureFixturesFacade {
  private:
    static constexpr double COORDINATE_LIMIT = 1.0e17;
    const ProceduralNoise * const proceduralNoise;
    const TextureUtils * const textureUtils;

  public:
    SolidTextureFixturesFacade(const ProceduralNoise *proceduralNoise, const TextureUtils *textureUtils);

    void checkerTexture(
        double x, double y, double z, ColorRgba *color,
        const SolidTextureFixturesCheckerParameterSet &parametersSet1,
        const SolidTextureFixturesCheckerParameterSet &parametersSet2,
        double smallTolerance) const;

    void colorAt(
        ColorRgba *color, int textureNumber,
        const Matrix4x4d *textureTransformationInverse, const ControlledRGBAImageHDRUncompressed *image,
        const ColorRgba *color1, const ColorRgba *color2, double turbulence, int octaves,
        const RGBAColorPalette *colorMap, Vector3Dd textureGradient, double mortar,
        const Vector3Dd *intersectionPoint, double smallTolerance,
        const SolidTextureFixturesColorAtParameterSet &parametersSet1 = SolidTextureFixturesColorAtParameterSet(),
        const SolidTextureFixturesColorAtParameterSet &parametersSet2 = SolidTextureFixturesColorAtParameterSet()) const;
};

#endif
