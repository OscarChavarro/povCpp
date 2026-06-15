#ifndef __SPHERE_H__
#define __SPHERE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Sphere : public Geometry {
  public:
    Vector3Dd Center;
    double Radius;
    double radiusSquared;
    double inverseRadius;
    Vector3Dd VPOtoC;
    double VPOCSquared;
    short VPinside;
    bool VPCached;
    bool Inverted;

    static int intersectSphere(const RayWithSegments *ray, Sphere *sphere,
        double *depth1, double *depth2);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;
};

#endif
