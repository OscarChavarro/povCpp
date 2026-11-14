#ifndef __RAY_SHADER_PIPELINE__
#define __RAY_SHADER_PIPELINE__

#include "render/PovRenderContext.h"
#include "render/shaders/TraceService.h"

class RayShaderPipeline {
  public:
    static void shadeSurface(IntersectionCandidate *rayIntersection,
        ColorRgba *color, const RayWithTracingState *ray, int shadowRay,
        const TraceService *traceService, TextureUtils *textureUtils,
        const PovRenderContext &context, int &traceLevel);
};

#endif
