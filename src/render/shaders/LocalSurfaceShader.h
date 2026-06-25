#ifndef __LOCAL_SURFACE_SHADER__
#define __LOCAL_SURFACE_SHADER__

#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/light/Light.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "java/util/ArrayList.h"
#include "render/shaders/TraceService.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"

class LocalSurfaceShader {
public:
    static void shade(const RayWithSegments *ray, PovRayMaterial *texture,
        IntersectionCandidate *rayIntersection, ColorRgba *surfaceColor,
        const ColorRgba *filterColor, ColorRgba *color,
        const TraceService *traceService, const java::ArrayList<Light*> &lightSources,
        const java::ArrayList<BoundedGeometry*> &objects,
        int &traceLevel, TextureUtils *textureUtils);
};

#endif
