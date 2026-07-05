#ifndef __TRANSMISSION_REFRACTION_SHADER__
#define __TRANSMISSION_REFRACTION_SHADER__

#include "environment/material/povray/PovRayMaterial.h"
#include "render/shaders/TraceService.h"

class TransmissionRefractionShader {
public:
    static void shade(PovRayMaterial *texture, const Vector3Dd *intersectionPoint,
        const RayWithTracingState *ray, const Vector3Dd *surfaceNormal,
        ColorRgba *color, const TraceService *traceService, double atmosphereIor,
        int &traceLevel);
};

#endif
