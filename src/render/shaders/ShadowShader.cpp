#include "render/shaders/ShadowShader.h"
#include "render/shaders/TraceService.h"
#include "common/color/Color.h"
#include "environment/geometry/Intersection.h"

int
ShadowShader::shade(Intersection *localIntersection,
    RGBAColor *lightColour, PriorityQueueNode *localQueue,
    const TraceService *traceService)
{
    traceService->shadeShadow(localIntersection, lightColour);

    if ((lightColour->Red < 0.01) && (lightColour->Green < 0.01) &&
        (lightColour->Blue < 0.01)) {

        while (localQueue->getHighest()) {
            localQueue->deleteHighest();
        }
        return TRUE;
    }
    return FALSE;
}
