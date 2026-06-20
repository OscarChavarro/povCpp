#ifndef __MIRROR_REFLECTION_SHADER__
#define __MIRROR_REFLECTION_SHADER__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/PovrayMaterial.h"
#include "render/shaders/TraceService.h"

class MirrorReflectionShader {
public:
    static void shade(const PovrayMaterial *texture, const Vector3Dd *intersectionPoint,
        const RayWithSegments *ray, const Vector3Dd *surfaceNormal,
        const TraceService *traceService);
};

#endif
