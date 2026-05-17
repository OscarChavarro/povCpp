#ifndef __TXTBUMP_H__
#define __TXTBUMP_H__

#include "common/Frame.h"
#include "common/Vector.h"
#include "media/Texture.h"

class BumpTextures {
  public:
    static void bumps(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal);
    static void dents(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal);
    static void ripples(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal);
    static void waves(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal);
    static void wrinkles(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal);
};

#endif
