#ifndef __PARAMETRIC_BI_CUBIC_SOLVER__
#define __PARAMETRIC_BI_CUBIC_SOLVER__

#include "environment/geometry/surface/parametric/ParametricBiCubicPatch.h"

class ParametricBiCubicSolver {
  public:
    static int intersectParametricBiCubicPatch0(const RayWithTracingState *ray,
        ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch1(const RayWithTracingState *ray,
        ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch2(const RayWithTracingState *ray,
        ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch3(const RayWithTracingState *ray,
        ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch4(const RayWithTracingState *ray,
        ParametricBiCubicPatch *shape, double *depths);
    static int allParametricBiCubicPatchIntersections(ParametricBiCubicPatch *shape,
        RayWithTracingState *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue);
};

#endif
