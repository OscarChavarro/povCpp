#ifndef __TXTBUMP_H__
#define __TXTBUMP_H__

#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

class BumpTextureFixture {
  public:
    static void bumps(
        double x, double y, double z, Texture *texture, Vector3Dd *normal,
        int debugEnabled);
    static void dents(
        double x, double y, double z, Texture *texture, Vector3Dd *normal,
        int debugEnabled);
    static void ripples(
        double x, double y, double z, Texture *texture, Vector3Dd *normal,
        int debugEnabled);
    static void waves(
        double x, double y, double z, Texture *texture, Vector3Dd *normal,
        int debugEnabled);
    static void wrinkles(
        double x, double y, double z, Texture *texture, Vector3Dd *normal,
        int debugEnabled);
};

#endif
