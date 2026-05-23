#ifndef __BLINN_PHONG_SPECULAR_SHADER_H__
#define __BLINN_PHONG_SPECULAR_SHADER_H__

#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

class RayWithSegments;
class RGBAColor;

class BlinnPhongSpecularShader {
public:
    static void shade(Texture *texture, RayWithSegments *lightSourceRay,
        Vector3Dd rEye, Vector3Dd *surfaceNormal, RGBAColor *colour,
        RGBAColor *lightColour, RGBAColor *surfaceColour);
};

#endif
