#ifndef __PHONG_SPECULAR_SHADER_H__
#define __PHONG_SPECULAR_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/material/Material.h"

class RayWithSegments;
class ColorRgba;

class PhongSpecularShader {
public:
    static void shade(const Material *texture, const RayWithSegments *lightSourceRay,
        Vector3Dd eye, const Vector3Dd *surfaceNormal, ColorRgba *color,
        const ColorRgba *lightColor, const ColorRgba *surfaceColor);
};

#endif
