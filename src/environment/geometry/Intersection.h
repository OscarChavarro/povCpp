#ifndef __Intersection
#define __Intersection

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/util/PriorityQueue.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/BoundedGeometry.h"

class SimpleBody;

class Intersection {
  public:
    double depth;
    BoundedGeometry *Object;
    Vector3Dd point;
    SimpleBody *Shape;

    inline bool operator<(const Intersection& other) const
    {
        return depth < other.depth;
    }
};

#endif // __Intersection
