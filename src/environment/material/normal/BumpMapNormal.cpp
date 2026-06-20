#include "environment/material/normal/BumpMapNormal.h"
#include "environment/geometry/GeometryConstants.h"

BumpMapNormal::BumpMapNormal(double bumpAmount, const ControlledRGBAImageHDRUncompressed *bumpImage) :
    bumpAmount(bumpAmount),
    bumpImage(bumpImage)
{
}

void
BumpMapNormal::applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
    const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    mapFixture.bumpMap(x, y, z, bumpImage, bumpAmount, newNormal, GeometryConstants::Small_Tolerance);
}

SolidTextureNormal *
BumpMapNormal::copy() const
{
    return new BumpMapNormal(bumpAmount, bumpImage);
}

double
BumpMapNormal::getBumpAmount() const
{
    return bumpAmount;
}

const ControlledRGBAImageHDRUncompressed *
BumpMapNormal::getBumpImage() const
{
    return bumpImage;
}
