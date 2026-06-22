#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "render/shaders/ShadowShader.h"
#include "render/shaders/TraceService.h"

int
ShadowShader::shade(IntersectionCandidate *localIntersection,
    ColorRgba *lightColor, java::PriorityQueue<IntersectionCandidate> *localDepthQueue,
    const TraceService *traceService)
{
    traceService->shadeShadow(localIntersection, lightColor);

    if ((lightColor->getR() < 0.01) && (lightColor->getG() < 0.01) &&
        (lightColor->getB() < 0.01)) {
        localDepthQueue->clear();
        return true;
    }
    return false;
}
