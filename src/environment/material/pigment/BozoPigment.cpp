#include "environment/material/pigment/BozoPigment.h"

BozoPigment::BozoPigment(double turbulence, int octaves, const RGBAColorPalette *colorMap) :
    turbulence(turbulence),
    octaves(octaves),
    colorMap(colorMap)
{
}

void
BozoPigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    colorFixture.bozo(x, y, z, turbulence, octaves, colorMap, color);
}

BozoPigment::~BozoPigment()
{
    delete colorMap;
}

SolidTexturePigment *
BozoPigment::copy() const
{
    return new BozoPigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap));
}

double
BozoPigment::getTurbulence() const
{
    return turbulence;
}

int
BozoPigment::getOctaves() const
{
    return octaves;
}

const RGBAColorPalette *
BozoPigment::getColorMap() const
{
    return colorMap;
}
