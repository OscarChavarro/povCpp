#include "render/SolidTextureFixturesCheckerParameterSet.h"

SolidTextureFixturesCheckerParameterSet::SolidTextureFixturesCheckerParameterSet()
    : textureNumber(0), textureTransformationInverse(nullptr), image(nullptr),
      color1(nullptr), color2(nullptr), turbulence(0.0), octaves(0),
      colorMap(nullptr), textureGradient(Vector3Dd()), mortar(0.0)
{
}

SolidTextureFixturesCheckerParameterSet::SolidTextureFixturesCheckerParameterSet(
    int textureNumber,
    const Matrix4x4d *textureTransformationInverse,
    const ControlledRGBAImageHDRUncompressed *image,
    const ColorRgba *color1,
    const ColorRgba *color2,
    double turbulence,
    int octaves,
    const RGBAColorPalette *colorMap,
    Vector3Dd textureGradient,
    double mortar)
    : textureNumber(textureNumber), textureTransformationInverse(textureTransformationInverse),
      image(image), color1(color1), color2(color2), turbulence(turbulence), octaves(octaves),
      colorMap(colorMap), textureGradient(textureGradient), mortar(mortar)
{
}

int
SolidTextureFixturesCheckerParameterSet::getTextureNumber() const
{
    return textureNumber;
}

const Matrix4x4d *
SolidTextureFixturesCheckerParameterSet::getTextureTransformationInverse() const
{
    return textureTransformationInverse;
}

const ControlledRGBAImageHDRUncompressed *
SolidTextureFixturesCheckerParameterSet::getImage() const
{
    return image;
}

const ColorRgba *
SolidTextureFixturesCheckerParameterSet::getColor1() const
{
    return color1;
}

const ColorRgba *
SolidTextureFixturesCheckerParameterSet::getColor2() const
{
    return color2;
}

double
SolidTextureFixturesCheckerParameterSet::getTurbulence() const
{
    return turbulence;
}

int
SolidTextureFixturesCheckerParameterSet::getOctaves() const
{
    return octaves;
}

const RGBAColorPalette *
SolidTextureFixturesCheckerParameterSet::getColorMap() const
{
    return colorMap;
}

Vector3Dd
SolidTextureFixturesCheckerParameterSet::getTextureGradient() const
{
    return textureGradient;
}

double
SolidTextureFixturesCheckerParameterSet::getMortar() const
{
    return mortar;
}
