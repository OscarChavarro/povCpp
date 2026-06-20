#include "environment/material/pigment/CheckerColorPigment.h"

CheckerColorPigment::CheckerColorPigment(ColorRgba *color1, ColorRgba *color2) :
    color1(color1),
    color2(color2)
{
}

void
CheckerColorPigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    colorFixture.checker(x, y, z, color, color1, color2, smallTolerance);
}

SolidTexturePigment *
CheckerColorPigment::copy() const
{
    return new CheckerColorPigment(color1, color2);
}

ColorRgba *
CheckerColorPigment::getColor1() const
{
    return color1;
}

ColorRgba *
CheckerColorPigment::getColor2() const
{
    return color2;
}
