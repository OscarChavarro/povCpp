#ifndef __PARAMETRIC_PATCH_NODE_H__
#define __PARAMETRIC_PATCH_NODE_H__

#include "common/linealAlgebra/Vector3Dd.h"

class ParametricPatchNode {
  public:
    int nodeType;
    Vector3Dd Center;
    double radiusSquared;
    int Count;
    void *Data_Ptr;
};

#endif
