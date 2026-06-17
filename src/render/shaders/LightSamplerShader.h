#ifndef __LIGHT_SAMPLER_SHADER_H__
#define __LIGHT_SAMPLER_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/light/Light.h"
#include "environment/geometry/element/RayWithSegments.h"

class LightSamplerShader {
public:
    static void sample(const Light *lightSource, double *lightSourceDepth,
        RayWithSegments *lightSourceRay, const Vector3Dd *intersectionPoint,
        ColorRgba *lightColor);
};

#endif
