#ifndef __BOX_TRACE__
#define __BOX_TRACE__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "environment/geometry/SurfaceTraceState.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL Box: the slab test with PovRay's intersection
// epsilon and render depth window; containment honors the inversion flag.
class BoxTrace {
  public:
    static bool intersect(const RayWithTracingState *ray, const Box &box,
        double *depth1, double *depth2);
    // A null context emits plain candidates with materialOverride.
    static int emitCrossings(Box &box, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    static bool firstHit(Box &box, RayWithTracingState *ray, IntersectionCandidate &out,
        Material *materialOverride);
    static int containment(const Box &box, const SurfaceTraceState &state,
        const Vector3Dd &point, double distanceTolerance);
};

#endif
