#ifndef __LAMBERT_SHADER__
#define __LAMBERT_SHADER__

#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/PovRayMaterial.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class LambertShader {
public:
    static void shade(const PovRayMaterial *texture, const RayWithSegments *lightSourceRay,
        const Vector3Dd *surfaceNormal, ColorRgba *color, const ColorRgba *lightColor,
        const ColorRgba *surfaceColor, double attenuation);
};

#endif
