#ifndef __DIRECT_LIGHT_SHADER_H__
#define __DIRECT_LIGHT_SHADER_H__

#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

class RayWithSegments;
class RGBAColor;
class TraceService;
class Light;
class SimpleBody;

class DirectLightShader {
public:
    static void shade(Texture *texture, Vector3Dd *intersectionPoint,
        RayWithSegments *eye, Vector3Dd *surfaceNormal,
        RGBAColor *surfaceColor, RGBAColor *color, double attenuation,
        const TraceService *traceService, Light *lightSources,
        SimpleBody *objects);
};

#endif
