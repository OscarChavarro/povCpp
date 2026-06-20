#ifndef __INTERSECTION__
#define __INTERSECTION__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/util/PriorityQueue.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/BoundedGeometry.h"

class SimpleBody;

class Intersection {
  private:
    double t;
    Vector3Dd point;
    Vector3Dd *normal = nullptr;

    BoundedGeometry *boundedGeometry;
    SimpleBody *ownerSimpleBody;

  public:
    double getT() const;
    void setT(double value);
    const Vector3Dd& getPoint() const;
    void setPoint(const Vector3Dd &value);
    Vector3Dd *getNormal() const;
    void setNormal(Vector3Dd *value);

    BoundedGeometry *getBoundedGeometry() const;
    void setBoundedGeometry(BoundedGeometry *value);
    Vector3Dd& getPoint();
    SimpleBody *getOwnerSimpleBody() const;
    void setOwnerSimpleBody(SimpleBody *value);
};

inline double
Intersection::getT() const
{
    return t;
}

inline void
Intersection::setT(double value)
{
    t = value;
}

inline const Vector3Dd&
Intersection::getPoint() const
{
    return point;
}

inline void
Intersection::setPoint(const Vector3Dd &value)
{
    point = value;
}

inline Vector3Dd *
Intersection::getNormal() const
{
    return normal;
}

inline void
Intersection::setNormal(Vector3Dd *value)
{
    normal = value;
}

inline BoundedGeometry *
Intersection::getBoundedGeometry() const
{
    return boundedGeometry;
}

inline void
Intersection::setBoundedGeometry(BoundedGeometry *value)
{
    boundedGeometry = value;
}

inline Vector3Dd&
Intersection::getPoint()
{
    return point;
}

inline SimpleBody *
Intersection::getOwnerSimpleBody() const
{
    return ownerSimpleBody;
}

inline void
Intersection::setOwnerSimpleBody(SimpleBody *value)
{
    ownerSimpleBody = value;
}

namespace java {
template <>
inline bool
PriorityQueue<Intersection>::lessThan(
    const Intersection& a, const Intersection& b) const
{
    return a.getT() < b.getT();
}
}

#endif
