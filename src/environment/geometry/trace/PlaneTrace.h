#ifndef __PLANE_TRACE__
#define __PLANE_TRACE__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "environment/geometry/SurfaceTraceState.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL PlanarHalfSpace: the plane depth with the
// per-viewpoint normal-origin product of primary rays and PovRay's depth
// window.
class PlaneTrace {
  public:
    static bool intersect(RayWithTracingState *ray, const PlanarHalfSpace &halfSpace,
        const SurfaceTraceState &state, double *depth);
    // A null context emits a plain candidate with materialOverride; otherwise
    // the candidate also carries the context's owner and local-point flag.
    static int emitCrossings(PlanarHalfSpace &halfSpace, const SurfaceTraceState &state,
        RayWithTracingState *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    static bool firstHit(PlanarHalfSpace &halfSpace, const SurfaceTraceState &state,
        RayWithTracingState *ray, IntersectionCandidate &out, Material *materialOverride);
    static int containment(const PlanarHalfSpace &halfSpace, const Vector3Dd &point,
        double distanceTolerance);
};

#endif
