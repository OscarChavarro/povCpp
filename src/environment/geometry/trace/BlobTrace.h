#ifndef __BLOB_TRACE__
#define __BLOB_TRACE__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"
#include "environment/geometry/SurfaceTraceState.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL BlobSolid: roots along the normalized direction
// with PovRay's shadow-ray root threshold and influence start; containment
// uses PovRay's density margin and honors the inversion flag.
class BlobTrace {
  public:
    static constexpr double INSIDE_TOLERANCE = 1.0e-6;
    static constexpr double SHADOW_ROOT_MIN_DISTANCE = 0.05;
    static constexpr double INFLUENCE_START = 0.01;

    // A null context emits plain candidates with materialOverride.
    static int emitCrossings(BlobSolid &solid, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    static int containment(const BlobSolid &solid, const SurfaceTraceState &state,
        const Vector3Dd &point, double distanceTolerance);
};

#endif
