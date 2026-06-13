#ifndef __LAMBERT_SHADER_H__
#define __LAMBERT_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/material/Material.h"

class LambertShader {
public:
    static void shade(const Material *texture, const RayWithSegments *lightSourceRay,
        const Vector3Dd *surfaceNormal, ColorRgba *color, const ColorRgba *lightColor,
        const ColorRgba *surfaceColor, double attenuation);
};

#endif
