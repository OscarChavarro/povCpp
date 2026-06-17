#ifndef __TRANSMISSION_REFRACTION_SHADER_H__
#define __TRANSMISSION_REFRACTION_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/PovrayMaterial.h"
#include "render/shaders/TraceService.h"

class TransmissionRefractionShader {
public:
    static void shade(PovrayMaterial *texture, const Vector3Dd *intersectionPoint,
        const RayWithSegments *ray, const Vector3Dd *surfaceNormal, ColorRgba *color,
        const TraceService *traceService, double atmosphereIor,
        int &traceLevel);
};

#endif
