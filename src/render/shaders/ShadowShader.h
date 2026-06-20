#ifndef __SHADOW_SHADER__
#define __SHADOW_SHADER__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/element/Intersection.h"
#include "render/shaders/TraceService.h"

class ShadowShader {
public:
    static int shade(Intersection *localIntersection,
        ColorRgba *lightColor, java::PriorityQueue<Intersection> *localQueue,
        const TraceService *traceService);
};

#endif
