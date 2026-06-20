#include "environment/material/pigment/LeopardPigment.h"

LeopardPigment::LeopardPigment(double turbulence, int octaves, const RGBAColorPalette *colorMap) :
    turbulence(turbulence),
    octaves(octaves),
    colorMap(colorMap)
{
}

void
LeopardPigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    colorFixture.leopard(x, y, z, turbulence, octaves, colorMap, color);
}

SolidTexturePigment *
LeopardPigment::copy() const
{
    return new LeopardPigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap));
}

double
LeopardPigment::getTurbulence() const
{
    return turbulence;
}

int
LeopardPigment::getOctaves() const
{
    return octaves;
}

const RGBAColorPalette *
LeopardPigment::getColorMap() const
{
    return colorMap;
}
