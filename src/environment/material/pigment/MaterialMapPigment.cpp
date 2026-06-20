#include "environment/material/pigment/MaterialMapPigment.h"

void
MaterialMapPigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
}

SolidTexturePigment *
MaterialMapPigment::copy() const
{
    return new MaterialMapPigment();
}
