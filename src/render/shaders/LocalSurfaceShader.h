#ifndef __LOCAL_SURFACE_SHADER_H__
#define __LOCAL_SURFACE_SHADER_H__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/light/Light.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/material/Material.h"
#include "render/shaders/TraceService.h"

class LocalSurfaceShader {
public:
    static void shade(const RayWithSegments *ray, Material *texture,
        Intersection *rayIntersection, ColorRgba *surfaceColor,
        const ColorRgba *filterColor, ColorRgba *color,
        const TraceService *traceService, const Light *lightSources,
        java::ArrayList<BoundedGeometry*> &objects, int &traceLevel);
};

#endif
