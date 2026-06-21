#ifndef __PHONG_SPECULAR_SHADER__
#define __PHONG_SPECULAR_SHADER__

#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class PhongSpecularShader {
public:
    static void shade(const PovRayMaterial *texture, const RayWithSegments *lightSourceRay,
        Vector3Dd eye, const Vector3Dd *surfaceNormal, ColorRgba *color,
        const ColorRgba *lightColor, const ColorRgba *surfaceColor);
};

#endif
