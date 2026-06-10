#ifndef __TXTBUMP_H__
#define __TXTBUMP_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "solidTexture/ProceduralNoise.h"
#include "solidTexture/Texture.h"

class BumpTextureFixture {
  public:
    BumpTextureFixture(ProceduralNoise *proceduralNoise);

    void bumps(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void dents(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void ripples(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void waves(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void wrinkles(double x, double y, double z, Texture *texture, Vector3Dd *normal);

  private:
    ProceduralNoise *proceduralNoise;
};

#endif
