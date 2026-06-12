#ifndef __LIGHT_SAMPLER_SHADER_H__
#define __LIGHT_SAMPLER_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Light;
class RayWithSegments;
class ColorRgba;

class LightSamplerShader {
public:
    static void sample(const Light *lightSource, double *lightSourceDepth,
        RayWithSegments *lightSourceRay, const Vector3Dd *intersectionPoint,
        ColorRgba *lightColor);
};

#endif
