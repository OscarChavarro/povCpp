#ifndef __TRANSMISSION_REFRACTION_SHADER_H__
#define __TRANSMISSION_REFRACTION_SHADER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"

class RayWithSegments;
class RGBAColor;
class TraceService;

class TransmissionRefractionShader {
public:
    static void shade(Texture *texture, Vector3Dd *intersectionPoint,
        RayWithSegments *ray, Vector3Dd *surfaceNormal, RGBAColor *color,
        const TraceService *traceService, double atmosphereIor,
        int &traceLevel);
};

#endif
