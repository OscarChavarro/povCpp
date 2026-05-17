#ifndef __PARAMETRIC_BICUBIC_SOLVER_H__
#define __PARAMETRIC_BICUBIC_SOLVER_H__

#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/geometry/GeometryOperations.h"

class ParametricBiCubicPatch;
class PriorityQueueNode;

class ParametricBiCubicSolver {
  public:
    static int intersectParametricBiCubicPatch0(RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch1(RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch2(RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch3(RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch4(RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths);
    static int allParametricBiCubicPatchIntersections(
        SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue);
};

#endif
