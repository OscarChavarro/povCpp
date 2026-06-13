#ifndef __BLINN_PHONG_SPECULAR_SHADER_H__
#define __BLINN_PHONG_SPECULAR_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/material/Material.h"

class BlinnPhongSpecularShader {
public:
    static void shade(const Material *texture, const RayWithSegments *lightSourceRay,
        Vector3Dd rEye, const Vector3Dd *surfaceNormal, ColorRgba *color,
        const ColorRgba *lightColor, const ColorRgba *surfaceColor);
};

#endif
