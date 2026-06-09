#ifndef __MIRROR_REFLECTION_SHADER_H__
#define __MIRROR_REFLECTION_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"

class RayWithSegments;
class RGBAColor;
class TraceService;

class MirrorReflectionShader {
public:
    static void shade(Texture *texture, Vector3Dd *intersectionPoint,
        RayWithSegments *ray, Vector3Dd *surfaceNormal, RGBAColor *color,
        const TraceService *traceService, int &traceLevel);
};

#endif
