#include "render/SolidTextureFixturesColorAtParameterSet.h"

SolidTextureFixturesColorAtParameterSet::SolidTextureFixturesColorAtParameterSet()
    : textureNumber(0), textureTransformationInverse(nullptr), image(nullptr),
      color1(nullptr), color2(nullptr), turbulence(0.0), octaves(0),
      colorMap(nullptr), textureGradient(Vector3Dd()), mortar(0.0)
{
}

SolidTextureFixturesColorAtParameterSet::SolidTextureFixturesColorAtParameterSet(
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
SolidTextureFixturesColorAtParameterSet::getTextureNumber() const
{
    return textureNumber;
}

const Matrix4x4d *
SolidTextureFixturesColorAtParameterSet::getTextureTransformationInverse() const
{
    return textureTransformationInverse;
}

const ControlledRGBAImageHDRUncompressed *
SolidTextureFixturesColorAtParameterSet::getImage() const
{
    return image;
}

const ColorRgba *
SolidTextureFixturesColorAtParameterSet::getColor1() const
{
    return color1;
}

const ColorRgba *
SolidTextureFixturesColorAtParameterSet::getColor2() const
{
    return color2;
}

double
SolidTextureFixturesColorAtParameterSet::getTurbulence() const
{
    return turbulence;
}

int
SolidTextureFixturesColorAtParameterSet::getOctaves() const
{
    return octaves;
}

const RGBAColorPalette *
SolidTextureFixturesColorAtParameterSet::getColorMap() const
{
    return colorMap;
}

Vector3Dd
SolidTextureFixturesColorAtParameterSet::getTextureGradient() const
{
    return textureGradient;
}

double
SolidTextureFixturesColorAtParameterSet::getMortar() const
{
    return mortar;
}
