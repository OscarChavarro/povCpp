#ifndef __TXTBUMP_H__
#define __TXTBUMP_H__

#include "solidTexture/procedural/ProceduralNoise.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

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
