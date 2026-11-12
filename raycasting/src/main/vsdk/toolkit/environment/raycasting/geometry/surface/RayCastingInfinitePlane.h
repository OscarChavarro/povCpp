#ifndef __POV_INFINITE_PLANE__
#define __POV_INFINITE_PLANE__

#include "vsdk/toolkit/environment/raycasting/geometry/RayCastingGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"

class RayCastingInfinitePlane : public RayCastingGeometry {
  public:
    RayCastingInfinitePlane();
    RayCastingInfinitePlane(const Vector3Dd &normalVector, double distance);
    RayCastingInfinitePlane(const Vector3Dd &normalVector, double distance,
        double vpNormDotOrigin, bool vpCached);
    RayCastingInfinitePlane(const RayCastingInfinitePlane &other) :
        halfSpace(other.halfSpace),
        vpNormDotOrigin(other.vpNormDotOrigin),
        vpCached(other.vpCached)
    {}

    Vector3Dd getNormalVector() const { return halfSpace.getBoundaryPlane().getCoefficients(); }
    void setNormalVector(const Vector3Dd& value) {
        halfSpace.getBoundaryPlane().setCoefficients(value, getDistance());
        vpCached = false;
    }
    double getDistance() const { return halfSpace.getBoundaryPlane().getConstantCoefficient(); }
    void setDistance(double d) {
        halfSpace.getBoundaryPlane().setCoefficients(getNormalVector(), d);
        vpCached = false;
    }
    const PlanarHalfSpace& getHalfSpace() const { return halfSpace; }
    const Geometry* getIntrinsicGeometry() const override { return &halfSpace; }
    Geometry* copyGeometry() const override { return halfSpace.copyGeometry(); }
    GeometryContainment classifyPoint(const Vector3Dd& point, double tolerance) const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override;
    RayFirstHitStatus querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
        RayHit* hit) const override { return halfSpace.querySurfaceDetail(ray, boundary, hit); }
    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* hit, RayBoundary* boundary = nullptr) const override;
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    double getVpNormDotOrigin() const { return vpNormDotOrigin; }
    void setVpNormDotOrigin(double value) { vpNormDotOrigin = value; }
    bool isVpCached() const { return vpCached; }
    void setVpCached(bool value) { vpCached = value; }

    static int intersectPlane(
        RayWithTracingState *ray, RayCastingInfinitePlane *plane, double *depth);

    void getBounds(GeometryBounds& result) const override;
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
    PlanarHalfSpace halfSpace;
    double vpNormDotOrigin;
    bool vpCached;
};

#endif
