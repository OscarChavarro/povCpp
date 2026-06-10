#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Intersection.h"
#include "render/shaders/ShadowShader.h"
#include "render/shaders/TraceService.h"

int
ShadowShader::shade(Intersection *localIntersection,
    ColorRgba *lightColor, PriorityQueueNode *localQueue,
    const TraceService *traceService)
{
    traceService->shadeShadow(localIntersection, lightColor);

    if ((lightColor->getR() < 0.01) && (lightColor->getG() < 0.01) &&
        (lightColor->getB() < 0.01)) {

        while (localQueue->getHighest()) {
            localQueue->deleteHighest();
        }
        return true;
    }
    return false;
}
