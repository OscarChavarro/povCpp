#ifndef __TXTTEST_H__
#define __TXTTEST_H__

#include "common/FrameConfig.h"
#include "common/Vector3D.h"
#include "media/Texture.h"

class TextureFixture {
  public:
    static void painted1(double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void painted2(double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void painted3(double x, double y, double z, Texture *texture, RGBAColor *colour);
    static void bumpy1(double x, double y, double z, Texture *texture, Vector3D *normal);
    static void bumpy2(double x, double y, double z, Texture *texture, Vector3D *normal);
    static void bumpy3(double x, double y, double z, Texture *texture, Vector3D *normal);
};

#endif
