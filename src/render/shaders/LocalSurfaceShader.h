#ifndef __LOCAL_SURFACE_SHADER_H__
#define __LOCAL_SURFACE_SHADER_H__

#include "environment/material/Material.h"

class RayWithSegments;
class Intersection;
class ColorRgba;
class TraceService;
class Light;
class SimpleBody;

class LocalSurfaceShader {
public:
    static void shade(const RayWithSegments *ray, Material *texture,
        Intersection *rayIntersection, ColorRgba *surfaceColor,
        const ColorRgba *filterColor, ColorRgba *color,
        const TraceService *traceService, const Light *lightSources,
        SimpleBody *objects, int &traceLevel);
};

#endif
