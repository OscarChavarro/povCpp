#include "environment/material/normal/WrinklesNormal.h"

WrinklesNormal::WrinklesNormal(double bumpAmount) :
    bumpAmount(bumpAmount)
{
}

void
WrinklesNormal::applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
    const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    bumpFixture.wrinkles(x, y, z, bumpAmount, newNormal);
}

SolidTextureNormal *
WrinklesNormal::copy() const
{
    return new WrinklesNormal(bumpAmount);
}

double
WrinklesNormal::getBumpAmount() const
{
    return bumpAmount;
}
