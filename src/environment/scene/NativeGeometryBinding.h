#ifndef VSDK_NATIVE_GEOMETRY_BINDING_H
#define VSDK_NATIVE_GEOMETRY_BINDING_H

#include "environment/geometry/RayCastingGeometry.h"

class Sphere;
class ProjectedTriangleTraversal;

// Transitional scene binding: owns one VITRAL geometry (a sphere, a triangle
// mesh with its projected traversal, or any other geometry traced through its
// own queries) and its POV trace state; the render trace is SurfaceTraceKernel.
class NativeGeometryBinding : public RayCastingGeometry {
private:
    Geometry* geometry;
    const Sphere* sphere;
    ProjectedTriangleTraversal* triangleTraversal;
    SurfaceTraceState traceState;
public:
    // Takes ownership. A binding requires a non-null geometry.
    explicit NativeGeometryBinding(Geometry* geometry, bool inverted = false);
    NativeGeometryBinding(const NativeGeometryBinding& other);
    ~NativeGeometryBinding() override;
    NativeGeometryBinding& operator=(const NativeGeometryBinding&) = delete;

    const Geometry* getIntrinsicGeometry() const override { return geometry; }
    Geometry* getDetailSurface() override { return geometry; }
    SurfaceDetailFamily getDetailFamily() const override;
    SurfaceTraceState* getTraceState() override { return &traceState; }
    bool isInverted() const { return traceState.isInverted(); }
    void invertGeometry() override;
    void enableProjectedTriangleTraversal();
    ProjectedTriangleTraversal* getProjectedTriangleTraversal() const { return triangleTraversal; }
    void* copy() override;
    GeometryContainment classifyPoint(const Vector3Dd& point, double tolerance) const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override;
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    void getBounds(GeometryBounds& result) const override;
    BoundingVolumeHierarchy* createBoundingVolume() const override;
    int doIntersectionForAllRayCrossings(RayWithTracingState* ray,
        java::PriorityQueue<IntersectionCandidate>* queue,
        Material* materialOverride = nullptr) override;
    int doIntersectionForAllRayCrossingsAnnotated(RayWithTracingState* ray,
        java::PriorityQueue<IntersectionCandidate>* queue,
        const GeometryIntersectionEmissionContext& context) override;
    bool hasNativeAnnotatedCrossings() const override { return true; }
    bool doIntersectionFirstHit(RayWithTracingState* ray,
        IntersectionCandidate& out, Material* materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd& point, double tolerance) override;
};

#endif
