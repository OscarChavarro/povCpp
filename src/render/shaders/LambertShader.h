#ifndef __LAMBERT_SHADER_H__
#define __LAMBERT_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "solidTexture/Texture.h"

class RayWithSegments;
class ColorRgba;

class LambertShader {
public:
    static void shade(Texture *texture, RayWithSegments *lightSourceRay,
        Vector3Dd *surfaceNormal, ColorRgba *color, ColorRgba *lightColor,
        ColorRgba *surfaceColor, double attenuation);
};

#endif
