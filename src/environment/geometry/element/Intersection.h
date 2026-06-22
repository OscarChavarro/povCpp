#ifndef __INTERSECTION__
#define __INTERSECTION__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Intersection {
  private:
    double t;
    Vector3Dd point;
    Vector3Dd normal;

  public:
    double getT() const;
    void setT(double value);
    const Vector3Dd& getPoint() const;
    void setPoint(const Vector3Dd &value);
    const Vector3Dd& getNormal() const;
    void setNormal(const Vector3Dd &value);

    Vector3Dd& getPoint();
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

inline const Vector3Dd&
Intersection::getNormal() const
{
    return normal;
}

inline void
Intersection::setNormal(const Vector3Dd &value)
{
    normal = value;
}

inline Vector3Dd&
Intersection::getPoint()
{
    return point;
}

#endif
