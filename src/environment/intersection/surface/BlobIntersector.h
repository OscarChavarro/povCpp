#ifndef __BLOB_INTERSECTOR__
#define __BLOB_INTERSECTOR__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"
#include "environment/intersection/SurfaceIntersectionState.h"
#include "environment/intersection/ray/RayWithTracingState.h"
#include "environment/intersection/hit/IntersectionCandidate.h"
#include "environment/intersection/hit/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL BlobSolid: roots along the normalized direction
// with PovRay's shadow-ray root threshold and influence start; containment
// uses PovRay's density margin and honors the inversion flag.
class BlobIntersector {
  public:
    static constexpr double INSIDE_TOLERANCE = 1.0e-6;
    static constexpr double SHADOW_ROOT_MIN_DISTANCE = 0.05;
    static constexpr double INFLUENCE_START = 0.01;

    // A null context emits plain candidates with materialOverride.
    static int emitCrossings(BlobSolid &solid, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    static int containment(const BlobSolid &solid, const SurfaceIntersectionState &state,
        const Vector3Dd &point, double distanceTolerance);
};

#endif
