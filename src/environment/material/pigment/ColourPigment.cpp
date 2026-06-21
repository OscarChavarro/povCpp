#include "environment/material/pigment/ColourPigment.h"

ColourPigment::ColourPigment(ColorRgba *color1) :
    color1(color1)
{
}

void
ColourPigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    color->setR(color->getR() + color1->getR());
    color->setG(color->getG() + color1->getG());
    color->setB(color->getB() + color1->getB());
    color->setA(color->getA() + color1->getA());
}

SolidTexturePigment *
ColourPigment::copy() const
{
    return new ColourPigment(new ColorRgba(*color1));
}

ColorRgba *
ColourPigment::getColor1() const
{
    return color1;
}

bool
ColourPigment::needsTransform() const
{
    return false;
}
