#ifndef __TXTTEST_H__
#define __TXTTEST_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"

class textureFixture {
  public:
    void painted1(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void painted2(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void painted3(double x, double y, double z, Texture *texture, RGBAColor *colour);
    void bumpy1(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void bumpy2(double x, double y, double z, Texture *texture, Vector3Dd *normal);
    void bumpy3(double x, double y, double z, Texture *texture, Vector3Dd *normal);
};

#endif
