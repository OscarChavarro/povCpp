#ifndef __BUMPS_NORMAL__
#define __BUMPS_NORMAL__

#include "environment/material/normal/SolidTextureNormal.h"

class BumpsNormal : public SolidTextureNormal {
  public:
    explicit BumpsNormal(double bumpAmount);

    void applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
        const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const override;
    SolidTextureNormal *copy() const override;
    double getBumpAmount() const;

  private:
    double bumpAmount;
};

#endif
