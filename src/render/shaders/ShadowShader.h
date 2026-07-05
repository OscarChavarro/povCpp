#ifndef __SHADOW_SHADER__
#define __SHADOW_SHADER__

#include "render/shaders/TraceService.h"

class ShadowShader {
public:
    static int shade(IntersectionCandidate *localIntersection,
        ColorRgba *lightColor, java::PriorityQueue<IntersectionCandidate> *localDepthQueue,
        const TraceService *traceService);
};

#endif
