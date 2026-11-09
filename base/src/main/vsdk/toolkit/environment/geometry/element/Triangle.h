#ifndef __TRIANGLE__
#define __TRIANGLE__

#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Intersection.h"

class Ray;

class Triangle
{
  private:
    int point0;
    int point1;
    int point2;
    Vector3Dd normal;

    static java::String intToStr(int val);

  public:
    Triangle();
    Triangle(int point0, int point1, int point2);

    int getPoint0() const;
    int getPoint1() const;
    int getPoint2() const;

    void setPoint0(int value);
    void setPoint1(int value);
    void setPoint2(int value);
    const Vector3Dd& getNormal() const;
    void setNormal(const Vector3Dd& normal);

    static Intersection* doIntersectionWithTriangle(
        const Ray& ray,
        const Vector3Dd& v0,
        const Vector3Dd& v1,
        const Vector3Dd& v2);
    static int containmentTest(
        const Vector3Dd& p0,
        const Vector3Dd& p1,
        const Vector3Dd& p2,
        const Vector3Dd& p,
        double distanceTolerance);
    static void minMax(
        const Vector3Dd& p0,
        const Vector3Dd& p1,
        const Vector3Dd& p2,
        double mm[6]);

    /**
    Provides an object to text report convertion, optimized for human
    readability and debugging. Do not use this method for serialization
    or persistence purposes.
    @return human readable representation of current triangle
    */
    java::String toString() const;
};

inline Triangle::Triangle() : point0(0), point1(0), point2(0), normal(0, 0, 0)
{
}

inline Triangle::Triangle(int point0, int point1, int point2)
    : point0(point0), point1(point1), point2(point2), normal(0, 0, 0)
{
}

inline int Triangle::getPoint0() const
{
    return point0;
}

inline int Triangle::getPoint1() const
{
    return point1;
}

inline int Triangle::getPoint2() const
{
    return point2;
}

inline void Triangle::setPoint0(int value)
{
    point0 = value;
}

inline void Triangle::setPoint1(int value)
{
    point1 = value;
}

inline void Triangle::setPoint2(int value)
{
    point2 = value;
}

inline const Vector3Dd& Triangle::getNormal() const
{
    return normal;
}

inline void Triangle::setNormal(const Vector3Dd& inNormal)
{
    normal = inNormal;
}

#endif
