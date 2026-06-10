#ifndef __LOCAL_SURFACE_SHADER_H__
#define __LOCAL_SURFACE_SHADER_H__

#include "solidTexture/Material.h"

class RayWithSegments;
class Intersection;
class ColorRgba;
class TraceService;
class Light;
class SimpleBody;

class LocalSurfaceShader {
public:
    static void shade(RayWithSegments *ray, Material *texture,
        Intersection *rayIntersection, ColorRgba *surfaceColor,
        ColorRgba *filterColor, ColorRgba *color,
        const TraceService *traceService, Light *lightSources,
        SimpleBody *objects, int &traceLevel);
};

#endif
