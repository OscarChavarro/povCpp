#ifndef __TXTTEST_H__
#define __TXTTEST_H__

#include "common/Frame.h"
#include "common/Vector.h"
#include "media/Texture.h"

class TestTextures {
  public:
    static void painted1(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void painted2(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void painted3(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static void bumpy1(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal);
    static void bumpy2(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal);
    static void bumpy3(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal);
};

#endif
