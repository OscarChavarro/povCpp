#ifndef __LAMBERT_SHADER_H__
#define __LAMBERT_SHADER_H__

#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

class RayWithSegments;
class RGBAColor;

class LambertShader {
public:
    static void shade(Texture *texture, RayWithSegments *lightSourceRay,
        Vector3Dd *surfaceNormal, RGBAColor *color, RGBAColor *lightColor,
        RGBAColor *surfaceColor, double attenuation);
};

#endif
