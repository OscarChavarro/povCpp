#ifndef __Intersection
#define __Intersection

#include "common/dataStructures/PriorityQueueNode.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class SimpleBody;
class Geometry;

class Intersection {
  public:
    double Depth;
    SimpleBody *Object;
    Vector3Dd Point;
    Geometry *Shape;
};

#endif // __Intersection
