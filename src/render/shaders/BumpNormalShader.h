#ifndef __BUMP_NORMAL_SHADER_H__
#define __BUMP_NORMAL_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/material/PovrayMaterial.h"

class BumpNormalShader {
  public:
    static void shade(
        Vector3Dd *newNormal,
        const PovrayMaterial *texture,
        const Vector3Dd *intersectionPoint,
        const Vector3Dd *surfaceNormal);
};

#endif
