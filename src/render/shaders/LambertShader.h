#ifndef __LAMBERT_SHADER__
#define __LAMBERT_SHADER__

#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/material/povray/PovRayMaterial.h"

class LambertShader {
public:
    static void shade(const PovRayMaterial *texture, const RayWithTracingState *lightSourceRay,
        const Vector3Dd *surfaceNormal, ColorRgba *color, const ColorRgba *lightColor,
        const ColorRgba *surfaceColor, double attenuation);
};

#endif
