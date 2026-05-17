#ifndef __Intersection
#define __Intersection

#include "environment/geometry/GeometryOperations.h"

class Intersection {
  public:
    double Depth;
    SimpleBody *Object;
    Vector3Dd Point;
    Geometry *Shape;
};

#endif // __Intersection
