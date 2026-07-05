#ifndef __LIGHT_SAMPLER_SHADER__
#define __LIGHT_SAMPLER_SHADER__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/light/Light.h"
#include "environment/geometry/element/RayWithTracingState.h"

class LightSamplerShader {
public:
    static void sample(const Light *lightSource, double *lightSourceDepth,
        RayWithTracingState *lightSourceRay, const Vector3Dd *intersectionPoint,
        ColorRgba *lightColor);
};

#endif
