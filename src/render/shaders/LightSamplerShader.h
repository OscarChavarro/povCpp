#ifndef __LIGHT_SAMPLER_SHADER__
#define __LIGHT_SAMPLER_SHADER__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/environment/light/Light.h"
#include "environment/geometry/element/RayWithTracingState.h"

class LightSamplerShader {
public:
    static void sample(const Light *lightSource, double *lightSourceDepth,
        RayWithTracingState *lightSourceRay, const Vector3Dd *intersectionPoint,
        ColorRgba *lightColor);
};

#endif
