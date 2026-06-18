#ifndef __INTERSECTION__
#define __INTERSECTION__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/util/PriorityQueue.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/BoundedGeometry.h"

class SimpleBody;

class Intersection {
  private:
    double depth;
    BoundedGeometry *Object;
    Vector3Dd point;
    SimpleBody *Shape;

  public:
    double getDepth() const { return depth; }
    void setDepth(double value) { depth = value; }
    BoundedGeometry *getObject() const { return Object; }
    void setObject(BoundedGeometry *value) { Object = value; }
    Vector3Dd& getPoint() { return point; }
    const Vector3Dd& getPoint() const { return point; }
    void setPoint(const Vector3Dd &value) { point = value; }
    SimpleBody *getShape() const { return Shape; }
    void setShape(SimpleBody *value) { Shape = value; }

    inline bool operator<(const Intersection& other) const
    {
        return getDepth() < other.getDepth();
    }
};

#endif
