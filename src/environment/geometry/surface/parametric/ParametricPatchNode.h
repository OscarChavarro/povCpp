#ifndef __PARAMETRIC_PATCH_NODE_H__
#define __PARAMETRIC_PATCH_NODE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class ParametricPatchNode {
  public:
    int nodeType;
    Vector3Dd center;
    double radiusSquared;
    int count;
    void *dataPtr;
};

#endif
