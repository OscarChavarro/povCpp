#ifndef __SURFACE_INTERSECTOR__
#define __SURFACE_INTERSECTOR__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "environment/intersection/SurfaceIntersectionState.h"
#include "environment/intersection/hit/SurfaceDetailFamily.h"
#include "environment/intersection/ray/RayWithTracingState.h"
#include "environment/intersection/hit/IntersectionCandidate.h"
#include "environment/intersection/hit/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL surface by family: crossing emission, first
// hit and containment with the family's POV state (SurfaceIntersectionState) and
// PovRay's tolerances. The family is fixed when the surface is bound to the
// scene, so the render chooses the intersector without asking the geometry. The
// family None has no intersector: callers trace such geometry themselves.
class SurfaceIntersector {
  public:
    // A null context emits plain candidates with materialOverride; with a
    // context, materialOverride must be the context's material override.
    static int emitCrossings(Geometry *surface, SurfaceDetailFamily family,
        SurfaceIntersectionState &state, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    // False for families without a direct first hit (the caller then takes
    // the nearest crossing) and on a miss.
    static bool firstHit(Geometry *surface, SurfaceDetailFamily family,
        SurfaceIntersectionState &state, RayWithTracingState *ray, IntersectionCandidate &out,
        Material *materialOverride);
    static int containment(Geometry *surface, SurfaceDetailFamily family,
        const SurfaceIntersectionState &state, const Vector3Dd &point, double distanceTolerance);
    // The family of a VITRAL geometry bound directly to a body (no adapter);
    // a triangle mesh without projected traversal is generic.
    static SurfaceDetailFamily familyOf(const Geometry *surface);
    // The parts of a trace state derived from a directly bound geometry.
    static void initializeState(const Geometry *surface, SurfaceDetailFamily family,
        SurfaceIntersectionState &state);
    // POV inverse of a family that encodes it in the geometry (quadric and
    // plane coefficients); a no-op where PovRay ignores inverse.
    static void invertGeometry(Geometry *surface, SurfaceDetailFamily family);
    // Whether the family keeps inversion as the trace state flag (box,
    // polynomial, blob, sphere, generic); the other families encode it in
    // their geometry or ignore it.
    static bool invertsByState(SurfaceDetailFamily family)
    {
        return family == SurfaceDetailFamily::Box || family == SurfaceDetailFamily::Polynomial ||
            family == SurfaceDetailFamily::Blob || family == SurfaceDetailFamily::Sphere ||
            family == SurfaceDetailFamily::Generic;
    }
};

#endif
