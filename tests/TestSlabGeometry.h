#ifndef VSDK_TEST_SLAB_GEOMETRY_H
#define VSDK_TEST_SLAB_GEOMETRY_H

#include "vsdk/toolkit/environment/geometry/Geometry.h"

// Minimal non-spherical contract fixture for testing the scene binding.
class TestSlabGeometry : public Geometry {
private:
    bool reversed;
public:
    explicit TestSlabGeometry(bool reversed = false) : reversed(reversed) {}
    Geometry* copyGeometry() const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override;
    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* hit, RayBoundary* boundary = nullptr) const override;
    bool doIntersectionFirstHit(const Ray& ray, RayHit* hit) override;
    void doExtraInformation(const Ray& ray, double t, RayHit* hit) override;
    GeometryContainment classifyPoint(const Vector3Dd& point, double tolerance) const override;
    void getBounds(GeometryBounds& bounds) const override;
    double* getMinMax() override;
};
#endif
