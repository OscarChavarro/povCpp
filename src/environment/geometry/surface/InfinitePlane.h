#ifndef __INFINITE_PLANE_H__
#define __INFINITE_PLANE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class InfinitePlane : public Geometry {
  public:
    Vector3Dd normalVector;
    double Distance;
    double VPNormDotOrigin;
    bool VPCached;

    static int intersectPlane(
        RayWithSegments *ray, InfinitePlane *plane, double *depth);

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
