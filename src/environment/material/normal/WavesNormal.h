#ifndef __WAVES_NORMAL__
#define __WAVES_NORMAL__

#include "environment/material/normal/SolidTextureNormal.h"

class WavesNormal : public SolidTextureNormal {
  public:
    WavesNormal(double bumpAmount, double frequency, double phase, int numberOfWaves);

    void applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
        const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const override;
    SolidTextureNormal *copy() const override;

  private:
    double bumpAmount;
    double frequency;
    double phase;
    int numberOfWaves;
};

#endif
