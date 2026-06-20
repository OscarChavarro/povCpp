#ifndef __MIRROR_REFLECTION_SHADER__
#define __MIRROR_REFLECTION_SHADER__

#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/PovRayMaterial.h"
#include "render/shaders/TraceService.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class MirrorReflectionShader {
public:
    static void shade(const PovRayMaterial *texture, const Vector3Dd *intersectionPoint,
        const RayWithSegments *ray, const Vector3Dd *surfaceNormal,
        ColorRgba *color, const TraceService *traceService, int &traceLevel);
};

#endif
