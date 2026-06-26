#include "environment/material/pigment/WoodPigment.h"

WoodPigment::WoodPigment(double turbulence, int octaves, const RGBAColorPalette *colorMap) :
    turbulence(turbulence),
    octaves(octaves),
    colorMap(colorMap)
{
}

void
WoodPigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    colorFixture.wood(x, y, z, turbulence, octaves, colorMap, color);
}

WoodPigment::~WoodPigment()
{
    delete colorMap;
}

SolidTexturePigment *
WoodPigment::copy() const
{
    return new WoodPigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap));
}

int
WoodPigment::getOctaves() const
{
    return octaves;
}
