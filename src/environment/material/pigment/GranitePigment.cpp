#include "environment/material/pigment/GranitePigment.h"

GranitePigment::GranitePigment(const RGBAColorPalette *colorMap) :
    colorMap(colorMap)
{
}

void
GranitePigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    colorFixture.granite(x, y, z, colorMap, color);
}

GranitePigment::~GranitePigment()
{
    delete colorMap;
}

SolidTexturePigment *
GranitePigment::copy() const
{
    return new GranitePigment(SolidTexturePigment::cloneColorMap(colorMap));
}

const RGBAColorPalette *
GranitePigment::getColorMap() const
{
    return colorMap;
}
