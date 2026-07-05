#ifndef __LOCAL_SURFACE_SHADER__
#define __LOCAL_SURFACE_SHADER__

#include "environment/scene/Scene.h"

class LocalSurfaceShader {
public:
    static void shade(const RayWithTracingState *ray, PovRayMaterial *texture,
        IntersectionCandidate *rayIntersection, ColorRgba *surfaceColor,
        const ColorRgba *filterColor, ColorRgba *color,
        const TraceService *traceService, const java::ArrayList<Light*> &lightSources,
        const BakedScene &bakedScene,
        int &traceLevel, TextureUtils *textureUtils);
};

#endif
