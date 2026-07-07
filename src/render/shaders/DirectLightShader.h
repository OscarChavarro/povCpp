#ifndef __DIRECT_LIGHT_SHADER__
#define __DIRECT_LIGHT_SHADER__

#include "vsdk/toolkit/environment/light/Light.h"
#include "render/bakedScene/BakedScene.h"
#include "render/shaders/TraceService.h"

class DirectLightShader {
public:
    static void shade(const PovRayMaterial *texture, const Vector3Dd *intersectionPoint,
        const RayWithTracingState *eye, const Vector3Dd *surfaceNormal,
        const ColorRgba *surfaceColor, ColorRgba *color, double attenuation,
        const TraceService *traceService, const java::ArrayList<Light*> &lightSources,
        const BakedScene &bakedScene);

private:
    static bool canUseCsgFirstHitForShadow(const BakedScene &bakedScene, int objectIndex);
    static bool traceShadowObject(
        const BakedScene &bakedScene,
        int objectIndex,
        RayWithTracingState *lightSourceRay,
        java::PriorityQueue<IntersectionCandidate> *localDepthQueue,
        double lightSourceDepth,
        ColorRgba *lightColor,
        const TraceService *traceService);
};

#endif
