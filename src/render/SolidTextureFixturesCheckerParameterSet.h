#ifndef __SOLID_TEXTURE_FIXTURES_CHECKER_PARAMETER_SET__
#define __SOLID_TEXTURE_FIXTURES_CHECKER_PARAMETER_SET__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"

class SolidTextureFixturesCheckerParameterSet {
  public:
    SolidTextureFixturesCheckerParameterSet();

    SolidTextureFixturesCheckerParameterSet(
        int textureNumber,
        const Matrix4x4d *textureTransformationInverse,
        const ControlledRGBAImageHDRUncompressed *image,
        const ColorRgba *color1,
        const ColorRgba *color2,
        double turbulence,
        int octaves,
        const RGBAColorPalette *colorMap,
        Vector3Dd textureGradient,
        double mortar);

    int getTextureNumber() const;
    const Matrix4x4d *getTextureTransformationInverse() const;
    const ControlledRGBAImageHDRUncompressed *getImage() const;
    const ColorRgba *getColor1() const;
    const ColorRgba *getColor2() const;
    double getTurbulence() const;
    int getOctaves() const;
    const RGBAColorPalette *getColorMap() const;
    Vector3Dd getTextureGradient() const;
    double getMortar() const;

  private:
    int textureNumber;
    const Matrix4x4d *textureTransformationInverse;
    const ControlledRGBAImageHDRUncompressed *image;
    const ColorRgba *color1;
    const ColorRgba *color2;
    double turbulence;
    int octaves;
    const RGBAColorPalette *colorMap;
    Vector3Dd textureGradient;
    double mortar;
};

#endif
