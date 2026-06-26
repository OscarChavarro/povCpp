#ifndef __WRINKLES_NORMAL__
#define __WRINKLES_NORMAL__

#include "environment/material/normal/SolidTextureNormal.h"

class WrinklesNormal : public SolidTextureNormal {
  public:
    explicit WrinklesNormal(double bumpAmount);

    void applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
        const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const override;
    SolidTextureNormal *copy() const override;

  private:
    double bumpAmount;
};

#endif
