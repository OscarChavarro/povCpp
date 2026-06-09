#ifndef __BLINN_PHONG_SPECULAR_SHADER_H__
#define __BLINN_PHONG_SPECULAR_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"

class RayWithSegments;
class ColorRgba;

class BlinnPhongSpecularShader {
public:
    static void shade(Texture *texture, RayWithSegments *lightSourceRay,
        Vector3Dd rEye, Vector3Dd *surfaceNormal, ColorRgba *color,
        ColorRgba *lightColor, ColorRgba *surfaceColor);
};

#endif
