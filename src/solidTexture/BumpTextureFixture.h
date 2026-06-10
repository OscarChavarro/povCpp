#ifndef __TXTBUMP_H__
#define __TXTBUMP_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "solidTexture/ProceduralNoise.h"

class BumpTextureFixture {
  public:
    BumpTextureFixture(ProceduralNoise *proceduralNoise);

    void bumps(
        double x, double y, double z, double bumpAmount, Vector3Dd *normal);
    void dents(
        double x, double y, double z, double bumpAmount, Vector3Dd *normal);
    void ripples(
        double x, double y, double z, double bumpAmount, double frequency,
        double phase, int numberOfWaves, Vector3Dd *normal);
    void waves(
        double x, double y, double z, double bumpAmount, double frequency,
        double phase, int numberOfWaves, Vector3Dd *normal);
    void wrinkles(
        double x, double y, double z, double bumpAmount, Vector3Dd *normal);

  private:
    ProceduralNoise *proceduralNoise;
};

#endif
