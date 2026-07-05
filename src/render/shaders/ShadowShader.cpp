#include "render/shaders/ShadowShader.h"
#include "java/util/PriorityQueue.txx"

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
