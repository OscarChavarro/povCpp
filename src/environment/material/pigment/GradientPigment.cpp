#include "environment/material/pigment/GradientPigment.h"

GradientPigment::GradientPigment(double turbulence, int octaves, const RGBAColorPalette *colorMap, const Vector3Dd &textureGradient) :
    turbulence(turbulence),
    octaves(octaves),
    colorMap(colorMap),
    textureGradient(textureGradient)
{
}

void
GradientPigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    colorFixture.gradient(x, y, z, turbulence, colorMap, textureGradient, octaves, color);
}

GradientPigment::~GradientPigment()
{
    delete colorMap;
}

SolidTexturePigment *
GradientPigment::copy() const
{
    return new GradientPigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap), textureGradient);
}

int
GradientPigment::getOctaves() const
{
    return octaves;
}

const Vector3Dd &
GradientPigment::getTextureGradient() const
{
    return textureGradient;
}
