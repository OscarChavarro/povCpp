#ifndef __BICUBIC_PATCH_TRACE__
#define __BICUBIC_PATCH_TRACE__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL SubdividedBezierPatch: PovRay shares a budget
// of MAX_BICUBIC_INTERSECTIONS hits per patch and thread between the rays
// traced since the last primary ray and keeps their points and normals in the
// patch's PatchScratch for the later normal query. A patch has no interior.
class BicubicPatchTrace {
  public:
    static constexpr int MAX_BICUBIC_INTERSECTIONS = SubdividedBezierPatch::MAXIMUM_HITS;

    // A null context emits plain candidates with materialOverride.
    static int emitCrossings(SubdividedBezierPatch &patch, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
};

#endif
