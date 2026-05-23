#ifndef __MIRROR_REFLECTION_SHADER_H__
#define __MIRROR_REFLECTION_SHADER_H__

#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

class RayWithSegments;
class RGBAColor;
class TraceService;

class MirrorReflectionShader {
public:
    static void shade(Texture *texture, Vector3Dd *intersectionPoint,
        RayWithSegments *ray, Vector3Dd *surfaceNormal, RGBAColor *colour,
        const TraceService *traceService);
};

#endif
