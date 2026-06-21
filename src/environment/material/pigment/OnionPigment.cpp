#include "environment/material/pigment/OnionPigment.h"

OnionPigment::OnionPigment(double turbulence, int octaves, const RGBAColorPalette *colorMap) :
    turbulence(turbulence),
    octaves(octaves),
    colorMap(colorMap)
{
}

void
OnionPigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    colorFixture.onion(x, y, z, turbulence, octaves, colorMap, color);
}

OnionPigment::~OnionPigment()
{
    delete colorMap;
}

SolidTexturePigment *
OnionPigment::copy() const
{
    return new OnionPigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap));
}

double
OnionPigment::getTurbulence() const
{
    return turbulence;
}

int
OnionPigment::getOctaves() const
{
    return octaves;
}

const RGBAColorPalette *
OnionPigment::getColorMap() const
{
    return colorMap;
}
