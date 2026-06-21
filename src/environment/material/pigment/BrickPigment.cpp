#include "environment/material/pigment/BrickPigment.h"

BrickPigment::BrickPigment(ColorRgba *color1, ColorRgba *color2, double mortar) :
    color1(color1),
    color2(color2),
    mortar(mortar)
{
}

void
BrickPigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    colorFixture.brick(x, y, z, color, color1, color2, mortar);
}

BrickPigment::~BrickPigment()
{
    delete color1;
    delete color2;
}

SolidTexturePigment *
BrickPigment::copy() const
{
    return new BrickPigment(new ColorRgba(*color1), new ColorRgba(*color2), mortar);
}

ColorRgba *
BrickPigment::getColor1() const
{
    return color1;
}

ColorRgba *
BrickPigment::getColor2() const
{
    return color2;
}

double
BrickPigment::getMortar() const
{
    return mortar;
}
