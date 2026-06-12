#ifndef __TRANSMISSION_REFRACTION_SHADER_H__
#define __TRANSMISSION_REFRACTION_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/material/Material.h"

class RayWithSegments;
class ColorRgba;
class TraceService;

class TransmissionRefractionShader {
public:
    static void shade(Material *texture, const Vector3Dd *intersectionPoint,
        const RayWithSegments *ray, const Vector3Dd *surfaceNormal, ColorRgba *color,
        const TraceService *traceService, double atmosphereIor,
        int &traceLevel);
};

#endif
