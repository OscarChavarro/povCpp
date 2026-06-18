#ifndef __INFINITE_PLANE__
#define __INFINITE_PLANE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"

class InfinitePlane : public Geometry {
  public:
    InfinitePlane();
    InfinitePlane(const Vector3Dd &normalVector, double distance);

    Vector3Dd &getNormalVector() { return normalVector; }
    const Vector3Dd &getNormalVector() const { return normalVector; }
    double getDistance() const { return distance; }
    void setDistance(double d) { distance = d; }
    double getVpNormDotOrigin() const { return vpNormDotOrigin; }
    void setVpNormDotOrigin(double value) { vpNormDotOrigin = value; }
    bool isVpCached() const { return vpCached; }
    void setVpCached(bool value) { vpCached = value; }

    static int intersectPlane(
        RayWithSegments *ray, InfinitePlane *plane, double *depth);

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

  private:
    Vector3Dd normalVector;
    double distance;
    double vpNormDotOrigin;
    bool vpCached;
};

#endif
