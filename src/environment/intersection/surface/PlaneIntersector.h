#ifndef __PLANE_INTERSECTOR__
#define __PLANE_INTERSECTOR__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "environment/intersection/SurfaceIntersectionState.h"
#include "environment/intersection/ray/RayWithTracingState.h"
#include "environment/intersection/hit/IntersectionCandidate.h"
#include "environment/intersection/hit/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL PlanarHalfSpace: the plane depth with the
// per-viewpoint normal-origin product of primary rays and PovRay's depth
// window.
class PlaneIntersector {
  public:
    static bool intersect(RayWithTracingState *ray, const PlanarHalfSpace &halfSpace,
        const SurfaceIntersectionState &state, double *depth);
    // A null context emits a plain candidate with materialOverride; otherwise
    // the candidate also carries the context's owner and local-point flag.
    static int emitCrossings(PlanarHalfSpace &halfSpace, const SurfaceIntersectionState &state,
        RayWithTracingState *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    static bool firstHit(PlanarHalfSpace &halfSpace, const SurfaceIntersectionState &state,
        RayWithTracingState *ray, IntersectionCandidate &out, Material *materialOverride);
    // POV inverse: the normal and the distance change sign.
    static void invert(PlanarHalfSpace &halfSpace);
    static int containment(const PlanarHalfSpace &halfSpace, const Vector3Dd &point,
        double distanceTolerance);
};

#endif
