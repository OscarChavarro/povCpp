#ifndef __Intersection
#define __Intersection

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/util/PriorityQueue.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/SimpleBody.h"

class TranslatedBody;

class Intersection {
  public:
    double depth;
    SimpleBody *Object;
    Vector3Dd point;
    TranslatedBody *Shape;

    inline bool operator<(const Intersection& other) const
    {
        return depth < other.depth;
    }
};

#endif // __Intersection
