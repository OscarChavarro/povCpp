#ifndef __POLYNOMIAL_INTERSECTOR__
#define __POLYNOMIAL_INTERSECTOR__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/volume/PolynomialSurface.h"
#include "environment/intersection/SurfaceIntersectionState.h"
#include "environment/intersection/ray/RayWithTracingState.h"
#include "environment/intersection/hit/IntersectionCandidate.h"
#include "environment/intersection/hit/GeometryIntersectionEmissionContext.h"

// POV render trace of a VITRAL PolynomialSurface: roots along the normalized
// direction with PovRay's shadow-ray root threshold; containment snaps values
// near the surface to zero and honors the inversion flag.
class PolynomialIntersector {
  public:
    static constexpr double SHADOW_ROOT_MIN_DISTANCE = 0.05;

    // A null context emits plain candidates with materialOverride.
    static int emitCrossings(PolynomialSurface &surface, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
    static int containment(const PolynomialSurface &surface, const SurfaceIntersectionState &state,
        const Vector3Dd &point, double distanceTolerance);
};

#endif
