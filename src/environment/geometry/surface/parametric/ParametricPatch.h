#ifndef __PARAMETRIC_PATCH_H__
#define __PARAMETRIC_PATCH_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class ParametricPatchConstants {
  public:
    static constexpr int PARAMETRIC_INTERIOR_NODE = 0;
    static constexpr int PARAMETRIC_LEAF_NODE = 1;
};

#include "environment/geometry/surface/parametric/ParametricBiCubicIntersection.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicPatch.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicSolver.h"
#include "environment/geometry/surface/parametric/ParametricControlPoints.h"
#include "environment/geometry/surface/parametric/ParametricPatchChild.h"
#include "environment/geometry/surface/parametric/ParametricPatchNode.h"

#endif
