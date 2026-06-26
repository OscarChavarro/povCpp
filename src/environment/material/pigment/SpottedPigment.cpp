#include "environment/material/pigment/SpottedPigment.h"

SpottedPigment::SpottedPigment(const RGBAColorPalette *colorMap) :
    colorMap(colorMap)
{
}

void
SpottedPigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    colorFixture.spotted(x, y, z, colorMap, color);
}

SpottedPigment::~SpottedPigment()
{
    delete colorMap;
}

SolidTexturePigment *
SpottedPigment::copy() const
{
    return new SpottedPigment(SolidTexturePigment::cloneColorMap(colorMap));
}
