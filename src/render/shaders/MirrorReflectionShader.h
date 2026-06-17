#ifndef __MIRROR_REFLECTION_SHADER_H__
#define __MIRROR_REFLECTION_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/material/PovrayMaterial.h"
#include "render/shaders/TraceService.h"

class MirrorReflectionShader {
public:
    static void shade(const PovrayMaterial *texture, const Vector3Dd *intersectionPoint,
        const RayWithSegments *ray, const Vector3Dd *surfaceNormal, ColorRgba *color,
        const TraceService *traceService, int &traceLevel);
};

#endif
