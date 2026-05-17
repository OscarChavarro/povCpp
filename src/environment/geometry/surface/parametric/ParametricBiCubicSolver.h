#ifndef __PARAMETRIC_BICUBIC_SOLVER_H__
#define __PARAMETRIC_BICUBIC_SOLVER_H__

#include "environment/geometry/elements/Ray.h"
#include "environment/geometry/GeometryOperations.h"

class ParametricBiCubicPatch;
class PriorityQueueNode;

class ParametricBiCubicSolver {
  public:
    static int intersectParametricBiCubicPatch0(Ray *ray, ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch1(Ray *ray, ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch2(Ray *ray, ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch3(Ray *ray, ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch4(Ray *ray, ParametricBiCubicPatch *shape, double *depths);
    static int allParametricBiCubicPatchIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
};

#endif
