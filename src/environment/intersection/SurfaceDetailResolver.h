#ifndef __SURFACE_DETAIL_RESOLVER__
#define __SURFACE_DETAIL_RESOLVER__

#include "environment/intersection/ray/RayWithTracingState.h"
#include "environment/intersection/hit/RayCastingHit.h"

// Detail evaluation of the POV render for a hit attributed to a VITRAL
// surface: the hit's family selects the evaluation, which reads the surface's
// local geometry (gradient, bounds, tangents, the patch hit cache) and applies
// PovRay's normalization and degenerate-normal rules. The ray and the hit
// point are in the surface's local frame.
class SurfaceDetailResolver {
  public:
    static void resolve(const RayWithTracingState &ray, double t, RayCastingHit *hit);
};

#endif
