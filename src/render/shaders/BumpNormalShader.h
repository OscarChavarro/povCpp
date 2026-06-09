#ifndef __BUMP_NORMAL_SHADER_H__
#define __BUMP_NORMAL_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"

class BumpNormalShader {
public:
    static void shade(Vector3Dd *newNormal, Texture *texture,
        Vector3Dd *intersectionPoint, Vector3Dd *surfaceNormal);
};

#endif
