#ifndef __POLYNOMIAL_TRACE__
#define __POLYNOMIAL_TRACE__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/volume/PolynomialSurface.h"
#include "environment/geometry/SurfaceTraceState.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL PolynomialSurface: roots along the normalized
// direction with PovRay's shadow-ray root threshold; containment snaps values
// near the surface to zero and honors the inversion flag.
class PolynomialTrace {
  public:
    static constexpr double SHADOW_ROOT_MIN_DISTANCE = 0.05;

    // A null context emits plain candidates with materialOverride.
    static int emitCrossings(PolynomialSurface &surface, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    static int containment(const PolynomialSurface &surface, const SurfaceTraceState &state,
        const Vector3Dd &point, double distanceTolerance);
};

#endif
