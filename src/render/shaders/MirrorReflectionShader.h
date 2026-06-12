#ifndef __MIRROR_REFLECTION_SHADER_H__
#define __MIRROR_REFLECTION_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/material/Material.h"

class RayWithSegments;
class ColorRgba;
class TraceService;

class MirrorReflectionShader {
public:
    static void shade(const Material *texture, const Vector3Dd *intersectionPoint,
        const RayWithSegments *ray, const Vector3Dd *surfaceNormal, ColorRgba *color,
        const TraceService *traceService, int &traceLevel);
};

#endif
