#ifndef __POV_INFINITE_PLANE__
#define __POV_INFINITE_PLANE__

#include "environment/geometry/RayCastingGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"

class RayCastingInfinitePlane : public RayCastingGeometry {
  public:
    RayCastingInfinitePlane();
    RayCastingInfinitePlane(const Vector3Dd &normalVector, double distance);
    RayCastingInfinitePlane(const Vector3Dd &normalVector, double distance,
        double vpNormDotOrigin, bool vpCached);
    RayCastingInfinitePlane(const RayCastingInfinitePlane &other) :
        halfSpace(other.halfSpace),
        traceState(other.traceState)
    {}

    Vector3Dd getNormalVector() const { return halfSpace.getBoundaryPlane().getCoefficients(); }
    void setNormalVector(const Vector3Dd& value) {
        halfSpace.getBoundaryPlane().setCoefficients(value, getDistance());
        traceState.setViewpointCached(false);
    }
    double getDistance() const { return halfSpace.getBoundaryPlane().getConstantCoefficient(); }
    void setDistance(double d) {
        halfSpace.getBoundaryPlane().setCoefficients(getNormalVector(), d);
        traceState.setViewpointCached(false);
    }
    const PlanarHalfSpace& getHalfSpace() const { return halfSpace; }
    const Geometry* getIntrinsicGeometry() const override { return &halfSpace; }
    Geometry* getDetailSurface() final { return &halfSpace; }
    SurfaceDetailFamily getDetailFamily() const final { return SurfaceDetailFamily::Plane; }
    SurfaceTraceState* getTraceState() final { return &traceState; }
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
    bool isVpCached() const { return traceState.isViewpointCached(); }

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

  private:
    PlanarHalfSpace halfSpace;
    SurfaceTraceState traceState;
};

#endif
