#ifndef __RAY_SHADER_PIPELINE__
#define __RAY_SHADER_PIPELINE__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "render/RenderContext.h"
#include "render/shaders/TraceService.h"

class RayShaderPipeline {
public:
    static void shadeSurface(Intersection *rayIntersection,
        ColorRgba *color, const RayWithSegments *ray, int shadowRay,
        const TraceService *traceService, TextureUtils *textureUtils,
        const RenderContext &context, int &traceLevel);
};

#endif
