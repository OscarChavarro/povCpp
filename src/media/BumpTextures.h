#ifndef __TXTBUMP_H__
#define __TXTBUMP_H__

#include "common/FrameConfig.h"
#include "common/Vector3D.h"
#include "media/Texture.h"

class BumpTextures {
  public:
    static void bumps(double x, double y, double z, Texture *texture, Vector3D *normal);
    static void dents(double x, double y, double z, Texture *texture, Vector3D *normal);
    static void ripples(double x, double y, double z, Texture *texture, Vector3D *normal);
    static void waves(double x, double y, double z, Texture *texture, Vector3D *normal);
    static void wrinkles(double x, double y, double z, Texture *texture, Vector3D *normal);
};

#endif
