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
    static void shade(Material *texture, Vector3Dd *intersectionPoint,
        RayWithSegments *eye, Vector3Dd *surfaceNormal,
        ColorRgba *surfaceColor, ColorRgba *color, double attenuation,
        const TraceService *traceService, Light *lightSources,
        SimpleBody *objects);
};

#endif
