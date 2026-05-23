#ifndef __PARAMETRIC_PATCH_NODE_H__
#define __PARAMETRIC_PATCH_NODE_H__

#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Vector3Dd.h"

class ParametricPatchNode {
  public:
    int Node_Type;
    Vector3Dd Center;
    double Radius_Squared;
    int Count;
    void *Data_Ptr;
};

#endif
