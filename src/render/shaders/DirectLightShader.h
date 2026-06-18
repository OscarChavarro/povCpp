#ifndef __DIRECT_LIGHT_SHADER__
#define __DIRECT_LIGHT_SHADER__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/light/Light.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/PovrayMaterial.h"
#include "render/shaders/TraceService.h"

class DirectLightShader {
public:
    static void shade(const PovrayMaterial *texture, const Vector3Dd *intersectionPoint,
        const RayWithSegments *eye, const Vector3Dd *surfaceNormal,
        const ColorRgba *surfaceColor, ColorRgba *color, double attenuation,
        const TraceService *traceService, const Light *lightSources,
        java::ArrayList<BoundedGeometry*> &objects);
};

#endif
