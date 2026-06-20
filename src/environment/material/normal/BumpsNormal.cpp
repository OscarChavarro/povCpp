#include "environment/material/normal/BumpsNormal.h"

BumpsNormal::BumpsNormal(double bumpAmount) :
    bumpAmount(bumpAmount)
{
}

void
BumpsNormal::applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
    const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    bumpFixture.bumps(x, y, z, bumpAmount, newNormal);
}

SolidTextureNormal *
BumpsNormal::copy() const
{
    return new BumpsNormal(bumpAmount);
}

double
BumpsNormal::getBumpAmount() const
{
    return bumpAmount;
}
