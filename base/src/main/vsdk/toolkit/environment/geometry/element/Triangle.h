#ifndef __TRIANGLE__
#define __TRIANGLE__

#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Intersection.h"

class Ray;

class Triangle
{
  private:
    int p0;
    int p1;
    int p2;
    Vector3Dd normal;

    static java::String intToStr(int val);

  public:
    Triangle();
    Triangle(int p0, int p1, int p2);

    int getPoint0() const;
    int getPoint1() const;
    int getPoint2() const;

    void setPoint0(int p0);
    void setPoint1(int p1);
    void setPoint2(int p2);
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

inline Triangle::Triangle() : p0(0), p1(0), p2(0), normal(0, 0, 0)
{
}

inline Triangle::Triangle(int inP0, int inP1, int inP2)
    : p0(inP0), p1(inP1), p2(inP2), normal(0, 0, 0)
{
}

inline int Triangle::getPoint0() const
{
    return p0;
}

inline int Triangle::getPoint1() const
{
    return p1;
}

inline int Triangle::getPoint2() const
{
    return p2;
}

inline void Triangle::setPoint0(int inP0)
{
    p0 = inP0;
}

inline void Triangle::setPoint1(int inP1)
{
    p1 = inP1;
}

inline void Triangle::setPoint2(int inP2)
{
    p2 = inP2;
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
