#ifndef __PARAMETRIC_BI_CUBIC_SOLVER__
#define __PARAMETRIC_BI_CUBIC_SOLVER__

#include "vsdk/toolkit/environment/raycasting/geometry/surface/parametric/RayCastingParametricBiCubicPatch.h"

class ParametricBiCubicSolver {
  public:
    static int intersectParametricBiCubicPatch0(const RayWithTracingState *ray,
        RayCastingParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch1(const RayWithTracingState *ray,
        RayCastingParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch2(const RayWithTracingState *ray,
        RayCastingParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch3(const RayWithTracingState *ray,
        RayCastingParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch4(const RayWithTracingState *ray,
        RayCastingParametricBiCubicPatch *shape, double *depths);
    static int allParametricBiCubicPatchIntersections(RayCastingParametricBiCubicPatch *shape,
        RayWithTracingState *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue);
};

#endif
