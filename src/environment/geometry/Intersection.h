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

    double getDepth() const { return depth; }
    BoundedGeometry *getObject() const { return Object; }
    Vector3Dd& getPoint() { return point; }
    const Vector3Dd& getPoint() const { return point; }
    SimpleBody *getShape() const { return Shape; }

    inline bool operator<(const Intersection& other) const
    {
        return depth < other.depth;
    }
};

#endif // __Intersection
