#ifndef __GEOMETRY__
#define __GEOMETRY__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/boundingVolumeHierarchy/BoundingVolumeHierarchy.h"
#include "environment/geometry/element/PostRayHitElement.h"
#include "environment/geometry/element/GeometryIntersectionEmissionContext.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/PovRayHit.h"
#include "vsdk/toolkit/processing/Containment.h"

class Geometry : public PostRayHitElement {
  public:
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
    virtual void doExtraInformation(const RayWithTracingState &ray, double t, PovRayHit *hit) override;
    virtual BoundingVolumeHierarchy *createBoundingVolume() const;
    virtual void *copy() = 0;
    virtual ~Geometry() {}

    virtual void invertGeometry() {}

  protected:
    virtual void computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint) {}
    virtual void computeSurfaceNormal(
        Vector3Dd *result,
        Vector3Dd *intersectionPoint,
        const RendererConfiguration *config)
    {
        (void)config;
        computeSurfaceNormal(result, intersectionPoint);
    }

  private:
    static void applyAnnotatedEmissionContext(
        IntersectionCandidate &candidate,
        const GeometryIntersectionEmissionContext &context);
};

#endif
