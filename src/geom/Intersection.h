#ifndef __Intersection
#define __Intersection

#include "geom/GeometryOperations.h"

class Intersection {
  public:
    double Depth;
    SimpleBody *Object;
    Vector3D Point;
    Geometry *Shape;
};

#endif // __Intersection
