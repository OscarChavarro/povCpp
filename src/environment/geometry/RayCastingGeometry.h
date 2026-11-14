#ifndef __POV_GEOMETRY__
#define __POV_GEOMETRY__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/boundingVolumeHierarchy/BoundingVolumeHierarchy.h"
#include "environment/geometry/element/SurfaceDetailFamily.h"
#include "environment/geometry/SurfaceTraceState.h"
#include "environment/geometry/element/GeometryIntersectionEmissionContext.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayCastingHit.h"
#include "vsdk/toolkit/processing/Containment.h"

class RayCastingGeometry : public Geometry {
  public:
    RayCrossingSupport emitRayCrossings(const Ray& ray,
        const RayCrossingQuery& query, RayCrossingSink& sink) const override;
    bool doIntersectionFirstHit(const Ray& ray, RayHit* hit) override;
    void doExtraInformation(const Ray& ray, double t, RayHit* hit) override;
    double* getMinMax() override;
    static constexpr int INSIDE = static_cast<int>(Containment::INSIDE);
    static constexpr int LIMIT = static_cast<int>(Containment::LIMIT);
    static constexpr int OUTSIDE = static_cast<int>(Containment::OUTSIDE);

    virtual int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) { (void)ray; (void)depthQueue; (void)materialOverride; return 0; }
    virtual int doIntersectionForAllRayCrossingsAnnotated(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context);
    virtual bool hasNativeAnnotatedCrossings() const { return false; }
    virtual bool doIntersectionFirstHit(
        RayWithTracingState *ray,
        IntersectionCandidate &out,
        Material *materialOverride = nullptr)
    {
        (void)ray;
        (void)out;
        (void)materialOverride;
        return false;
    }
    virtual int doContainmentTest(const Vector3Dd &point, double distanceTolerance) { (void)point; (void)distanceTolerance; return OUTSIDE; }
    virtual BoundingVolumeHierarchy *createBoundingVolume() const;
    // The leaf used for mathematical provenance may outlive a single query,
    // but is owned by this adapter and has the same lifetime as the adapter.
    virtual const Geometry *getIntrinsicGeometry() const { return this; }
    // The VITRAL surface a hit on this geometry is attributed to, and the
    // family of its detail kernel; none for geometries whose hits come from
    // their operands (CSG) or that keep the emitted normal (light geometry).
    virtual Geometry *getDetailSurface() { return nullptr; }
    virtual SurfaceDetailFamily getDetailFamily() const { return SurfaceDetailFamily::None; }
    // The POV render state the family trace kernel uses for this surface.
    virtual SurfaceTraceState *getTraceState() { return nullptr; }
    virtual void *copy() = 0;
    virtual ~RayCastingGeometry() {}

    virtual void invertGeometry() {}

  private:
    static void applyAnnotatedEmissionContext(
        IntersectionCandidate &candidate,
        const GeometryIntersectionEmissionContext &context);
};

#endif
