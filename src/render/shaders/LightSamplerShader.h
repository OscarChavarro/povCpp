#ifndef __LIGHT_SAMPLER_SHADER_H__
#define __LIGHT_SAMPLER_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Light;
class RayWithSegments;
class RGBAColor;

class LightSamplerShader {
public:
    static void sample(Light *lightSource, double *lightSourceDepth,
        RayWithSegments *lightSourceRay, Vector3Dd *intersectionPoint,
        RGBAColor *lightColor);
};

#endif
