#ifndef __TRACE_WORKSPACE__
#define __TRACE_WORKSPACE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
class TraceWorkspace {
public:
    static const int DEFAULT_MAX_RECURSION_LEVEL = 8;

    RayHit traversalCandidateHit;
    RayHit nearestHit;
    RayHit shadowCandidateHitObj;
    java::ArrayList<RayHit> reflectionHits;
    java::ArrayList<RayHit> shadingHits;
    java::ArrayList<ColorRgb> reflectionColors;

    TraceWorkspace();
    explicit TraceWorkspace(int maxRecursionLevel);

    RayHit* shadowCandidateHit();
};

#endif
