#ifndef __SHADOW_SHADER_H__
#define __SHADOW_SHADER_H__

#include "common/dataStructures/PriorityQueueNode.h"

class Intersection;
class RGBAColor;
class TraceService;

class ShadowShader {
public:
    static int shade(Intersection *localIntersection,
        RGBAColor *lightColor, PriorityQueueNode *localQueue,
        const TraceService *traceService);
};

#endif
