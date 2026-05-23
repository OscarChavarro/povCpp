#ifndef __PHONG_SPECULAR_SHADER_H__
#define __PHONG_SPECULAR_SHADER_H__

#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

class RayWithSegments;
class RGBAColor;

class PhongSpecularShader {
public:
    static void shade(Texture *texture, RayWithSegments *lightSourceRay,
        Vector3Dd eye, Vector3Dd *surfaceNormal, RGBAColor *color,
        RGBAColor *lightColor, RGBAColor *surfaceColor);
};

#endif
