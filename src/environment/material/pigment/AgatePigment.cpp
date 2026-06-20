#include "environment/material/pigment/AgatePigment.h"

AgatePigment::AgatePigment(int octaves, const RGBAColorPalette *colorMap) :
    octaves(octaves),
    colorMap(colorMap)
{
}

void
AgatePigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    colorFixture.agate(x, y, z, octaves, colorMap, color);
}

SolidTexturePigment *
AgatePigment::copy() const
{
    return new AgatePigment(octaves, SolidTexturePigment::cloneColorMap(colorMap));
}

int
AgatePigment::getOctaves() const
{
    return octaves;
}

const RGBAColorPalette *
AgatePigment::getColorMap() const
{
    return colorMap;
}
