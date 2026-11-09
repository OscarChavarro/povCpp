#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/render/TraceWorkspace.h"
TraceWorkspace::TraceWorkspace() : TraceWorkspace(DEFAULT_MAX_RECURSION_LEVEL)
{
}

TraceWorkspace::TraceWorkspace(int maxRecursionLevel)
    : traversalCandidateHit(RayHit::DETAIL_NONE, false),
      nearestHit(RayHit::DETAIL_NONE, false),
      shadowCandidateHitObj(RayHit::DETAIL_NONE, false)
{
    int levels = maxRecursionLevel + 1;
    reflectionHits.reserve((long int)levels);
    shadingHits.reserve((long int)levels);
    reflectionColors.reserve((long int)levels);
    for ( int i = 0; i < levels; i++ ) {
        reflectionHits.add(RayHit(RayHit::DETAIL_NONE, false));
        shadingHits.add(RayHit());
        reflectionColors.add(ColorRgb());
    }
}

RayHit* TraceWorkspace::shadowCandidateHit()
{
    return &shadowCandidateHitObj;
}
