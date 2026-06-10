#ifndef __PHONG_SPECULAR_SHADER_H__
#define __PHONG_SPECULAR_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "solidTexture/Material.h"

class RayWithSegments;
class ColorRgba;

class PhongSpecularShader {
public:
    static void shade(Material *texture, RayWithSegments *lightSourceRay,
        Vector3Dd eye, Vector3Dd *surfaceNormal, ColorRgba *color,
        ColorRgba *lightColor, ColorRgba *surfaceColor);
};

#endif
