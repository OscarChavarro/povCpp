#ifndef __INFINITE_PLANE__
#define __INFINITE_PLANE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"

class InfinitePlane : public Geometry {
  public:
    InfinitePlane();
    InfinitePlane(const Vector3Dd &normalVector, double distance);
    InfinitePlane(const Vector3Dd &normalVector, double distance,
        double vpNormDotOrigin, bool vpCached);
    InfinitePlane(const InfinitePlane &other) :
        normalVector(other.normalVector),
        distance(other.distance),
        vpNormDotOrigin(other.vpNormDotOrigin),
        vpCached(other.vpCached)
    {}

    Vector3Dd &getNormalVector() { return normalVector; }
    const Vector3Dd &getNormalVector() const { return normalVector; }
    double getDistance() const { return distance; }
    void setDistance(double d) { distance = d; }
    double getVpNormDotOrigin() const { return vpNormDotOrigin; }
    void setVpNormDotOrigin(double value) { vpNormDotOrigin = value; }
    bool isVpCached() const { return vpCached; }
    void setVpCached(bool value) { vpCached = value; }

    static int intersectPlane(
        RayWithTracingState *ray, InfinitePlane *plane, double *depth);

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    bool doIntersectionFirstHit(
        RayWithTracingState *ray,
        IntersectionCandidate &out,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    void invertGeometry() override;

  protected:
    void computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;

  private:
    Vector3Dd normalVector;
    double distance;
    double vpNormDotOrigin;
    bool vpCached;
};

#endif
