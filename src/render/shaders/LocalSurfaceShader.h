#ifndef __LOCAL_SURFACE_SHADER_H__
#define __LOCAL_SURFACE_SHADER_H__

#include "media/Texture.h"

class RayWithSegments;
class Intersection;
class RGBAColor;
class TraceService;
class RenderFrame;

class LocalSurfaceShader {
public:
    static void shade(RayWithSegments *ray, Texture *texture,
        Intersection *rayIntersection, RGBAColor *surfaceColor,
        RGBAColor *filterColor, RGBAColor *color,
        const TraceService *traceService, const RenderFrame &frame,
        int &traceLevel);
};

#endif
