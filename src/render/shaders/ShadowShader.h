#ifndef __SHADOW_SHADER_H__
#define __SHADOW_SHADER_H__

#include "common/dataStructures/PriorityQueueNode.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Intersection.h"
#include "render/shaders/TraceService.h"

class ShadowShader {
public:
    static int shade(Intersection *localIntersection,
        ColorRgba *lightColor, PriorityQueueNode *localQueue,
        const TraceService *traceService);
};

#endif
