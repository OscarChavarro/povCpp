#ifndef __PARAMETRIC_BICUBIC_SOLVER_H__
#define __PARAMETRIC_BICUBIC_SOLVER_H__

#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicPatch.h"

class ParametricBiCubicSolver {
  public:
    static int intersectParametricBiCubicPatch0(const RayWithSegments *ray,
        ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch1(const RayWithSegments *ray,
        ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch2(const RayWithSegments *ray,
        ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch3(const RayWithSegments *ray,
        ParametricBiCubicPatch *shape, double *depths);
    static int intersectParametricBiCubicPatch4(const RayWithSegments *ray,
        ParametricBiCubicPatch *shape, double *depths);
    static int allParametricBiCubicPatchIntersections(BoundedGeometry *object,
        RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue);
};

#endif
