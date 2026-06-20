#ifndef __RIPPLES_NORMAL__
#define __RIPPLES_NORMAL__

#include "environment/material/normal/SolidTextureNormal.h"

class RipplesNormal : public SolidTextureNormal {
  public:
    RipplesNormal(double bumpAmount, double frequency, double phase, int numberOfWaves);

    void applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
        const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const override;
    SolidTextureNormal *copy() const override;
    double getBumpAmount() const;
    double getFrequency() const;
    double getPhase() const;
    int getNumberOfWaves() const;

  private:
    double bumpAmount;
    double frequency;
    double phase;
    int numberOfWaves;
};

#endif
