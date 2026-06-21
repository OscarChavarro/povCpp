#ifndef __TRANSMISSION_REFRACTION_SHADER__
#define __TRANSMISSION_REFRACTION_SHADER__

#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "render/shaders/TraceService.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class TransmissionRefractionShader {
public:
    static void shade(PovRayMaterial *texture, const Vector3Dd *intersectionPoint,
        const RayWithSegments *ray, const Vector3Dd *surfaceNormal,
        ColorRgba *color, const TraceService *traceService, double atmosphereIor,
        int &traceLevel);
};

#endif
