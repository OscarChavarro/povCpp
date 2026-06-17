#ifndef __PHONG_SPECULAR_SHADER_H__
#define __PHONG_SPECULAR_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/PovrayMaterial.h"

class PhongSpecularShader {
public:
    static void shade(const PovrayMaterial *texture, const RayWithSegments *lightSourceRay,
        Vector3Dd eye, const Vector3Dd *surfaceNormal, ColorRgba *color,
        const ColorRgba *lightColor, const ColorRgba *surfaceColor);
};

#endif
