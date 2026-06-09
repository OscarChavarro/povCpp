#ifndef __TXTBUMP_H__
#define __TXTBUMP_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"

class bumpTextureFixture {
  public:
    void bumps(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void dents(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void ripples(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void waves(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void wrinkles(double x, double y, double z, Texture *texture, Vector3Dd *normal);
};

#endif
