#ifndef __SPHERE_H__
#define __SPHERE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Sphere : public Geometry {
  public:
    Vector3Dd center;
    double radius;
    double radiusSquared;
    double inverseRadius;
    Vector3Dd vpOtoC;
    double vpOCSquared;
    short vpInside;
    bool vpCached;
    bool inverted;

    Vector3Dd& getCenter() { return center; }
    const Vector3Dd& getCenter() const { return center; }
    double getRadius() const { return radius; }
    void setRadius(double r) { radius = r; }
    double getRadiusSquared() const { return radiusSquared; }
    void setRadiusSquared(double value) { radiusSquared = value; }
    double getInverseRadius() const { return inverseRadius; }
    void setInverseRadius(double ir) { inverseRadius = ir; }

    static int intersectSphere(const RayWithSegments *ray, Sphere *sphere,
        double *depth1, double *depth2);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int allIntersectionsForOwner(
        RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue,
        SimpleBody *owner) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;
};

#endif
