#include "environment/material/normal/DentsNormal.h"

DentsNormal::DentsNormal(double bumpAmount) :
    bumpAmount(bumpAmount)
{
}

void
DentsNormal::applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
    const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    bumpFixture.dents(x, y, z, bumpAmount, newNormal);
}

SolidTextureNormal *
DentsNormal::copy() const
{
    return new DentsNormal(bumpAmount);
}
