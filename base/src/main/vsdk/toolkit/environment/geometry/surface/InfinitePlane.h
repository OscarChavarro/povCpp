#ifndef __INFINITE_PLANE__
#define __INFINITE_PLANE__

#include "java/lang/Double.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/environment/geometry/surface/HalfSpace.h"
class Ray;
class RayHit;

class InfinitePlane : public HalfSpace {
private:
    // Plane equation ax + by + cz + d = 0.
    double a;
    double b;
    double c;
    double d;
    RayFirstHitStatus evaluateRay(const Ray& ray, const RayCrossingQuery& query,
        double& depth) const;
    Vector3Dd surfaceNormal() const;

public:
    InfinitePlane(const InfinitePlane& other);
    InfinitePlane(double a, double b, double c, double d);
    InfinitePlane(const Vector3Dd& normal, const Vector3Dd& pointInPlane);
    InfinitePlane(const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2);

    void clone(const InfinitePlane& other);
    Geometry* copyGeometry() const override { return new InfinitePlane(*this); }
    void getBounds(GeometryBounds& result) const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override;
    RayFirstHitStatus querySurfaceDetail(const Ray& ray, const RayBoundary& boundary, RayHit* hit) const override;
    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* result, RayBoundary* boundary = nullptr) const override;

    // Intersect a plane from its precomputed signed numerator and directional
    // denominator. The caller supplies the parallel and interval policies.
    static RayFirstHitStatus intersectionDepth(double numerator, double denominator,
        double parallelTolerance, double minT, double maxT, bool checkArithmetic,
        double& depth);

    Ray* doIntersectionFirstHit(const Ray& inout_rayo);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);

    Ray* doIntersectionWithNegative(const Ray& inout_rayo);

    // Legacy predicates in unnormalized plane units (Java parity). The half-space
    // form classifies n.p + d <= 0; the surface form reports LIMIT on the plane
    // and OUTSIDE elsewhere (its -INSIDE equals OUTSIDE).
    int doContainmentTestHalfSpace(const Vector3Dd& p, double distanceTolerance);
    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);

    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);
    virtual double* getMinMax();

    // Raw coefficients preserve the equation scale; getNormal() is normalized.
    Vector3Dd getCoefficients() const { return Vector3Dd(a, b, c); }
    double getConstantCoefficient() const { return d; }
    void setCoefficients(const Vector3Dd& normal, double constant) {
        a = normal.x(); b = normal.y(); c = normal.z(); d = constant;
    }
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

inline RayFirstHitStatus InfinitePlane::intersectionDepth(double numerator, double denominator,
    double parallelTolerance, double minT, double maxT, bool checkArithmetic, double& depth)
{
    if (checkArithmetic && (!java::Double::isFinite(numerator) ||
        !java::Double::isFinite(denominator) || !java::Double::isFinite(parallelTolerance) ||
        parallelTolerance < 0 || java::Double::isNaN(minT) || java::Double::isNaN(maxT) || minT > maxT))
        return RayFirstHitStatus::InvalidInput;
    const bool parallel = parallelTolerance == 0 ? denominator == 0 :
        denominator < parallelTolerance && denominator > -parallelTolerance;
    if (parallel) return checkArithmetic && numerator == 0 ?
        RayFirstHitStatus::Indeterminate : RayFirstHitStatus::Miss;
    depth = numerator / denominator;
    if (checkArithmetic && !java::Double::isFinite(depth)) return RayFirstHitStatus::Indeterminate;
    return depth >= minT && depth <= maxT ? RayFirstHitStatus::Hit : RayFirstHitStatus::Miss;
}

#endif
