#ifndef __BLINN_PHONG_SPECULAR_SHADER__
#define __BLINN_PHONG_SPECULAR_SHADER__

#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class BlinnPhongSpecularShader {
public:
    static void shade(const PovRayMaterial *texture, const RayWithTracingState *lightSourceRay,
        Vector3Dd rEye, const Vector3Dd *surfaceNormal, ColorRgba *color,
        const ColorRgba *lightColor, const ColorRgba *surfaceColor);
};

#endif
