#ifndef __TRANSMISSION_REFRACTION_SHADER_H__
#define __TRANSMISSION_REFRACTION_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "solidTexture/Material.h"

class RayWithSegments;
class ColorRgba;
class TraceService;

class TransmissionRefractionShader {
public:
    static void shade(Material *texture, Vector3Dd *intersectionPoint,
        RayWithSegments *ray, Vector3Dd *surfaceNormal, ColorRgba *color,
        const TraceService *traceService, double atmosphereIor,
        int &traceLevel);
};

#endif
