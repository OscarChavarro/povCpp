#ifndef __BUMP_MAP_NORMAL__
#define __BUMP_MAP_NORMAL__

#include "environment/material/normal/SolidTextureNormal.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"

class BumpMapNormal : public SolidTextureNormal {
  public:
    BumpMapNormal(double bumpAmount, const ControlledRGBAImageHDRUncompressed *bumpImage);

    void applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
        const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const override;
    SolidTextureNormal *copy() const override;

  private:
    double bumpAmount;
    const ControlledRGBAImageHDRUncompressed *bumpImage;
};

#endif
