#ifndef __QUADRIC__
#define __QUADRIC__

#include "environment/geometry/RayCastingGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "environment/geometry/SurfaceTraceState.h"

class Quadric : public RayCastingGeometry {
  public:
    Quadric();
    Quadric(const Vector3Dd &object2Terms, const Vector3Dd &objectMixedTerms,
        const Vector3Dd &objectTerms, double objectConstant);
    Quadric(const Vector3Dd &object2Terms, const Vector3Dd &objectMixedTerms,
        const Vector3Dd &objectTerms, double objectConstant,
        double objectVpConstant, bool constantCached, bool nonZeroSquareTerm);
    Quadric(const Quadric &other) :
        surface(other.surface),
        traceState(other.traceState)
    {}

    Vector3Dd &getObject2Terms() { return surface.squareCoefficients(); }
    const Vector3Dd &getObject2Terms() const { return surface.squareCoefficients(); }
    Vector3Dd &getObjectMixedTerms() { return surface.mixedCoefficients(); }
    const Vector3Dd &getObjectMixedTerms() const { return surface.mixedCoefficients(); }
    Vector3Dd &getObjectTerms() { return surface.linearCoefficients(); }
    const Vector3Dd &getObjectTerms() const { return surface.linearCoefficients(); }
    double getObjectConstant() const { return surface.constantCoefficient(); }
    const QuadricSurface &getSurface() const { return surface; }
    bool isConstantCached() const { return traceState.isViewpointCached(); }
    void setConstantCached(bool value) const { traceState.setViewpointCached(value); }
    bool hasNonZeroSquareTerm() const { return traceState.hasNonZeroSquareTerm(); }

    const Geometry* getIntrinsicGeometry() const override { return &surface; }
    Geometry* getDetailSurface() final { return &surface; }
    SurfaceDetailFamily getDetailFamily() const final { return SurfaceDetailFamily::Quadric; }
    SurfaceTraceState* getTraceState() final { return &traceState; }
    Geometry* copyGeometry() const override { return surface.copyGeometry(); }
    void getBounds(GeometryBounds& result) const override { surface.getBounds(result); }
    GeometryContainment classifyPoint(const Vector3Dd& point, double tolerance) const override {
        return surface.classifyPoint(point, tolerance);
    }
    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* hit, RayBoundary* boundary = nullptr) const override {
        return surface.queryFirstHit(ray, query, hit, boundary);
    }
    RayFirstHitStatus querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
        RayHit* hit) const override { return surface.querySurfaceDetail(ray, boundary, hit); }
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override { return surface.emitRayCrossings(ray, query, sink); }
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override { return surface.classifyRay(ray, query, result); }

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doIntersectionForAllRayCrossingsAnnotated(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context) override;
    bool hasNativeAnnotatedCrossings() const override { return true; }
    bool doIntersectionFirstHit(
        RayWithTracingState *ray,
        IntersectionCandidate &out,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    BoundingVolumeHierarchy *createBoundingVolume() const override;
    void *copy() override;
    void invertGeometry() override;

  protected:

  private:
    void updateSquareTermFlag();

    QuadricSurface surface;
    SurfaceTraceState traceState;
};
#endif
