#ifndef __BUMP_TEXTURE_FIXTURE_H__
#define __BUMP_TEXTURE_FIXTURE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/solidTexture/procedural/ProceduralNoise.h"
class BumpTextureFixture {
  private:
    const ProceduralNoise * const proceduralNoise;

  public:
    BumpTextureFixture(const ProceduralNoise *proceduralNoise);

    void bumps(
        double x, double y, double z, double bumpAmount, Vector3Dd *normal) const;
    void dents(
        double x, double y, double z, double bumpAmount, Vector3Dd *normal) const;
    void ripples(
        double x, double y, double z, double bumpAmount, double frequency,
        double phase, int numberOfWaves, Vector3Dd *normal) const;
    void waves(
        double x, double y, double z, double bumpAmount, double frequency,
        double phase, int numberOfWaves, Vector3Dd *normal) const;
    void wrinkles(
        double x, double y, double z, double bumpAmount, Vector3Dd *normal) const;
};

#endif
