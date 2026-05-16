#ifndef __BEZIER_NODE_H__
#define __BEZIER_NODE_H__

#include "common/Frame.h"
#include "common/Vector.h"

class BezierNode {
  public:
    int Node_Type;
    Vector3D Center;
    DBL Radius_Squared;
    int Count;
    void *Data_Ptr;
};

#endif
