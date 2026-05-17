#ifndef __BEZIER_NODE_H__
#define __BEZIER_NODE_H__

#include "common/FrameConfig.h"
#include "common/Vector3D.h"

class BezierNode {
  public:
    int Node_Type;
    Vector3D Center;
    double Radius_Squared;
    int Count;
    void *Data_Ptr;
};

#endif
