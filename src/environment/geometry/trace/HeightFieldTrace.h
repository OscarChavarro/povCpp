#ifndef __HEIGHT_FIELD_TRACE__
#define __HEIGHT_FIELD_TRACE__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL HeightFieldSolid in its grid coordinates:
// the first hit of the block traversal, with PovRay's statistics, and PovRay
// 1.0's integer-truncated containment test.
class HeightFieldTrace {
  public:
    // A null context emits a plain candidate with materialOverride.
    static int emitCrossings(HeightFieldSolid &solid, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    static int containment(const HeightFieldSolid &solid, const Vector3Dd &point,
        double distanceTolerance);
};

#endif
