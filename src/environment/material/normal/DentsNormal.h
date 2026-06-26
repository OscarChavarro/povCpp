#ifndef __DENTS_NORMAL__
#define __DENTS_NORMAL__

#include "environment/material/normal/SolidTextureNormal.h"

class DentsNormal : public SolidTextureNormal {
  public:
    explicit DentsNormal(double bumpAmount);

    void applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
        const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const override;
    SolidTextureNormal *copy() const override;

  private:
    double bumpAmount;
};

#endif
