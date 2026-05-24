#include "render/shaders/ShadowShader.h"
#include "render/shaders/TraceService.h"
#include "common/color/Color.h"
#include "environment/geometry/Intersection.h"

int
ShadowShader::shade(Intersection *localIntersection,
    RGBAColor *lightColor, PriorityQueueNode *localQueue,
    const TraceService *traceService)
{
    traceService->shadeShadow(localIntersection, lightColor);

    if ((lightColor->Red < 0.01) && (lightColor->Green < 0.01) &&
        (lightColor->Blue < 0.01)) {

        while (localQueue->getHighest()) {
            localQueue->deleteHighest();
        }
        return LegacyBoolean::TRUE_VALUE;
    }
    return LegacyBoolean::FALSE_VALUE;
}
