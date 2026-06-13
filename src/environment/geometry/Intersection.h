#ifndef __Intersection
#define __Intersection

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/dataStructures/PriorityQueueNode.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/SimpleBody.h"

class Intersection {
  public:
    double Depth;
    SimpleBody *Object;
    Vector3Dd Point;
    Geometry *Shape;
};

#endif // __Intersection
