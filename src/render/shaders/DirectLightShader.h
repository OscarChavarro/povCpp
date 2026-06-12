#ifndef __DIRECT_LIGHT_SHADER_H__
#define __DIRECT_LIGHT_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/material/Material.h"

class RayWithSegments;
class ColorRgba;
class TraceService;
class Light;
class SimpleBody;

class DirectLightShader {
public:
    static void shade(const Material *texture, const Vector3Dd *intersectionPoint,
        const RayWithSegments *eye, const Vector3Dd *surfaceNormal,
        const ColorRgba *surfaceColor, ColorRgba *color, double attenuation,
        const TraceService *traceService, const Light *lightSources,
        SimpleBody *objects);
};

#endif
