#ifndef __BEZIER_NODE_H__
#define __BEZIER_NODE_H__

#include "common/FrameConfig.h"
#include "common/linealAlgebra/Vector3Dd.h"

class BezierNode {
  public:
    int Node_Type;
    Vector3Dd Center;
    double Radius_Squared;
    int Count;
    void *Data_Ptr;
};

#endif
