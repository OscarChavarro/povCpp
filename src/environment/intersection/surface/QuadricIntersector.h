#ifndef __QUADRIC_INTERSECTOR__
#define __QUADRIC_INTERSECTOR__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "environment/intersection/SurfaceIntersectionState.h"
#include "environment/intersection/ray/RayWithTracingState.h"
#include "environment/intersection/hit/IntersectionCandidate.h"
#include "environment/intersection/hit/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL QuadricSurface: the ray expansion with the
// ray's cached products, the per-viewpoint constant term of primary rays and
// PovRay's render depth window.
class QuadricIntersector {
  public:
    static bool intersect(RayWithTracingState *ray, const QuadricSurface &surface,
        const SurfaceIntersectionState &state, double *depth1, double *depth2);
    // PovRay's render depth window [SMALL_TOLERANCE, MAX_DISTANCE]: a root
    // outside it takes the other root's value; false when both are outside.
    static bool clipToRenderDepths(double *depth1, double *depth2);
    // A null context emits plain candidates with materialOverride.
    static int emitCrossings(QuadricSurface &surface, const SurfaceIntersectionState &state,
        RayWithTracingState *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    static bool firstHit(QuadricSurface &surface, const SurfaceIntersectionState &state,
        RayWithTracingState *ray, IntersectionCandidate &out, Material *materialOverride);
    static int containment(const QuadricSurface &surface, const Vector3Dd &point,
        double distanceTolerance);
    // Whether any square or mixed coefficient is nonzero.
    static bool hasNonZeroSquareTerm(const QuadricSurface &surface);
    // POV inverse: every coefficient changes sign.
    static void invert(QuadricSurface &surface);
};

#endif
