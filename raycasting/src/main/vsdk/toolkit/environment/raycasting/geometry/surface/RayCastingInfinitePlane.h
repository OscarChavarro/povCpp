#ifndef __POV_INFINITE_PLANE__
#define __POV_INFINITE_PLANE__

#include "vsdk/toolkit/environment/raycasting/geometry/RayCastingGeometry.h"

class RayCastingInfinitePlane : public RayCastingGeometry {
  public:
    RayCastingInfinitePlane();
    RayCastingInfinitePlane(const Vector3Dd &normalVector, double distance);
    RayCastingInfinitePlane(const Vector3Dd &normalVector, double distance,
        double vpNormDotOrigin, bool vpCached);
    RayCastingInfinitePlane(const RayCastingInfinitePlane &other) :
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
        RayWithTracingState *ray, RayCastingInfinitePlane *plane, double *depth);

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
