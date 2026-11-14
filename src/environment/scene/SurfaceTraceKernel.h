#ifndef __SURFACE_TRACE_KERNEL__
#define __SURFACE_TRACE_KERNEL__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "environment/geometry/SurfaceTraceState.h"
#include "environment/geometry/element/SurfaceDetailFamily.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL surface by family: crossing emission, first
// hit and containment with the family's POV state (SurfaceTraceState) and
// PovRay's tolerances. The family is fixed when the surface is bound to the
// scene, so the render chooses the kernel without asking the geometry. The
// family None has no kernel: callers trace such geometry themselves.
class SurfaceTraceKernel {
  public:
    // A null context emits plain candidates with materialOverride; with a
    // context, materialOverride must be the context's material override.
    static int emitCrossings(Geometry *surface, SurfaceDetailFamily family,
        SurfaceTraceState &state, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    // False for families without a direct first hit (the caller then takes
    // the nearest crossing) and on a miss.
    static bool firstHit(Geometry *surface, SurfaceDetailFamily family,
        SurfaceTraceState &state, RayWithTracingState *ray, IntersectionCandidate &out,
        Material *materialOverride);
    static int containment(Geometry *surface, SurfaceDetailFamily family,
        const SurfaceTraceState &state, const Vector3Dd &point, double distanceTolerance);
};

#endif
