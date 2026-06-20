#include "environment/material/pigment/MarblePigment.h"

MarblePigment::MarblePigment(double turbulence, int octaves, const RGBAColorPalette *colorMap) :
    turbulence(turbulence),
    octaves(octaves),
    colorMap(colorMap)
{
}

void
MarblePigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    colorFixture.marble(x, y, z, turbulence, octaves, colorMap, color);
}

SolidTexturePigment *
MarblePigment::copy() const
{
    return new MarblePigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap));
}

double
MarblePigment::getTurbulence() const
{
    return turbulence;
}

int
MarblePigment::getOctaves() const
{
    return octaves;
}

const RGBAColorPalette *
MarblePigment::getColorMap() const
{
    return colorMap;
}
