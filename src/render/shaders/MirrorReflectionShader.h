#ifndef __MIRROR_REFLECTION_SHADER__
#define __MIRROR_REFLECTION_SHADER__

#include "environment/material/povray/PovRayMaterial.h"
#include "render/shaders/TraceService.h"

class MirrorReflectionShader {
public:
    static void shade(const PovRayMaterial *texture, const Vector3Dd *intersectionPoint,
        const RayWithTracingState *ray, const Vector3Dd *surfaceNormal,
        ColorRgba *color, const TraceService *traceService, int &traceLevel);
};

#endif
