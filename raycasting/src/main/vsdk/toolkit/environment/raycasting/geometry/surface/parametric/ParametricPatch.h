#ifndef __PARAMETRIC_PATCH_CONSTANTS__
#define __PARAMETRIC_PATCH_CONSTANTS__


class ParametricPatchConstants {
  public:
    static constexpr int PARAMETRIC_INTERIOR_NODE = 0;
    static constexpr int PARAMETRIC_LEAF_NODE = 1;
};

#include "vsdk/toolkit/environment/raycasting/geometry/surface/parametric/ParametricBiCubicIntersection.h"
#include "vsdk/toolkit/environment/raycasting/geometry/surface/parametric/ParametricBiCubicSolver.h"

#endif
