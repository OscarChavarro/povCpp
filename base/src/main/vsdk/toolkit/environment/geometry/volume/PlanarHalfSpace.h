#ifndef VSDK_PLANAR_HALF_SPACE_H
#define VSDK_PLANAR_HALF_SPACE_H

#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"

// The closed solid n dot p + d <= 0. The plane owns its mathematical
// coefficients; placement and complement are independent scene operations.
class PlanarHalfSpace : public Solid {
private:
    InfinitePlane plane;
public:
    PlanarHalfSpace(double a, double b, double c, double d);
    explicit PlanarHalfSpace(const InfinitePlane& boundary);
    Geometry* copyGeometry() const override;
    const InfinitePlane& getBoundaryPlane() const { return plane; }
    InfinitePlane& getBoundaryPlane() { return plane; }
    void getBounds(GeometryBounds& result) const override;
    GeometryContainment classifyPoint(const Vector3Dd& point, double tolerance) const override;
    int doContainmentTest(const Vector3Dd& point, double tolerance) override;
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override;
    RayFirstHitStatus querySurfaceDetail(const Ray& ray, const RayBoundary& boundary, RayHit* hit) const override;
    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* hit, RayBoundary* boundary = nullptr) const override;
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    bool doIntersectionFirstHit(const Ray& ray, RayHit* hit) override;
    void doExtraInformation(const Ray& ray, double t, RayHit* hit) override;
    double* getMinMax() override;
};
#endif
