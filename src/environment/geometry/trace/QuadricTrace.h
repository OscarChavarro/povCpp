#ifndef __QUADRIC_TRACE__
#define __QUADRIC_TRACE__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "environment/geometry/SurfaceTraceState.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL QuadricSurface: the ray expansion with the
// ray's cached products, the per-viewpoint constant term of primary rays and
// PovRay's render depth window.
class QuadricTrace {
  public:
    static bool intersect(RayWithTracingState *ray, const QuadricSurface &surface,
        const SurfaceTraceState &state, double *depth1, double *depth2);
    // PovRay's render depth window [SMALL_TOLERANCE, MAX_DISTANCE]: a root
    // outside it takes the other root's value; false when both are outside.
    static bool clipToRenderDepths(double *depth1, double *depth2);
    // A null context emits plain candidates with materialOverride.
    static int emitCrossings(QuadricSurface &surface, const SurfaceTraceState &state,
        RayWithTracingState *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    static bool firstHit(QuadricSurface &surface, const SurfaceTraceState &state,
        RayWithTracingState *ray, IntersectionCandidate &out, Material *materialOverride);
    static int containment(const QuadricSurface &surface, const Vector3Dd &point,
        double distanceTolerance);
};

#endif
