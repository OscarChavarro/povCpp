#ifndef __INFINITE_PLANE__
#define __INFINITE_PLANE__

#include "java/lang/String.h"
#include "vsdk/toolkit/environment/geometry/surface/HalfSpace.h"
class Ray;
class RayHit;

class InfinitePlane : public HalfSpace {
private:
    // This is the infinite plane with canonical equation ax + bx + cx + d = 0
    double a;
    double b;
    double c;
    double d;

public:
    InfinitePlane(const InfinitePlane& other);
    InfinitePlane(double a, double b, double c, double d);
    InfinitePlane(const Vector3Dd& normal, const Vector3Dd& pointInPlane);
    InfinitePlane(const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2);

    void clone(const InfinitePlane& other);

    Ray* doIntersectionFirstHit(const Ray& inout_rayo);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);

    Ray* doIntersectionWithNegative(const Ray& inout_rayo);

    int doContainmentTestHalfSpace(const Vector3Dd& p, double distanceTolerance);
    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);

    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);
    virtual double* getMinMax();

    Vector3Dd getNormal() const;
    double getD() const;

    void setNormal(const Vector3Dd& n);
    void setD(double d);
    void setFromPointNormal(const Vector3Dd& p, const Vector3Dd& n);

    double pointDistance(const Vector3Dd& p) const;
    Vector3Dd projectPoint(const Vector3Dd& p) const;
    Vector3Dd mirrorPoint(const Vector3Dd& p) const;

    bool overlapsWith(const InfinitePlane& other, double tolerance) const;
    java::String toString() const;

    double getA() const;
    void setA(double a);
    double getB() const;
    void setB(double b);
    double getC() const;
    void setC(double c);
};

#endif
