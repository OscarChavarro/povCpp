#ifndef __BUMP_NORMAL_SHADER_H__
#define __BUMP_NORMAL_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "solidTexture/Material.h"

class BumpNormalShader {
public:
    static void shade(Vector3Dd *newNormal, Material *texture,
        Vector3Dd *intersectionPoint, Vector3Dd *surfaceNormal);
};

#endif
