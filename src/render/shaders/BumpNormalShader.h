#ifndef __BUMP_NORMAL_SHADER__
#define __BUMP_NORMAL_SHADER__

#include "environment/material/povray/PovRayMaterial.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"

class BumpNormalShader {
  public:
    static void shade(
        Vector3Dd *newNormal,
        const PovRayMaterial *texture,
        const Vector3Dd *intersectionPoint,
        const Vector3Dd *surfaceNormal,
        TextureUtils *textureUtils);
};

#endif
