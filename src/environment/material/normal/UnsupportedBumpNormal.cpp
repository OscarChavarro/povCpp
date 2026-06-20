#include "environment/material/normal/UnsupportedBumpNormal.h"

void
UnsupportedBumpNormal::applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
    const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const
{
}

SolidTextureNormal *
UnsupportedBumpNormal::copy() const
{
    return new UnsupportedBumpNormal();
}
