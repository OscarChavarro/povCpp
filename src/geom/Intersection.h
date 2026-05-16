#ifndef __Intersection
#define __Intersection

#include "geom/Geometry.h"

class Intersection {
  public:
    DBL Depth;
    SimpleBody *Object;
    Vector3D Point;
    Geometry *Shape;
};

#endif // __Intersection
