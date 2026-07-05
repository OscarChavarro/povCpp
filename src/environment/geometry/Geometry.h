#ifndef __GEOMETRY__
#define __GEOMETRY__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/util/PriorityQueue.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/geometry/element/AxisAlignedBoundingBox.h"
#include "environment/material/PovRayRendererConfiguration.h"
#include "environment/material/Material.h"
#include "vsdk/toolkit/processing/Containment.h"

class IntersectionCandidate;
class PovRayHit;
class RayOperationOwner;

struct GeometryIntersectionEmissionContext {
    Material *materialOverride = nullptr;
    RayOperationOwner *detailOwner = nullptr;
    bool materialUsesObjectLocalPoint = false;
};

class Geometry {
  public:
    static constexpr int INSIDE = static_cast<int>(Containment::INSIDE);
    static constexpr int LIMIT = static_cast<int>(Containment::LIMIT);
    static constexpr int OUTSIDE = static_cast<int>(Containment::OUTSIDE);

    virtual int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) { (void)ray; (void)depthQueue; (void)materialOverride; return 0; }
    virtual int doIntersectionForAllRayCrossingsAnnotated(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context);
    virtual bool hasNativeAnnotatedCrossings() const { return false; }
    virtual Geometry *getWrappedGeometry() const { return nullptr; }
    bool doIntersectionFirstHitViaCrossings(RayWithSegments *ray, IntersectionCandidate &out);
    virtual bool doIntersectionFirstHit(
        RayWithSegments *ray,
        IntersectionCandidate &out,
        Material *materialOverride = nullptr)
    {
        (void)ray;
        (void)out;
        (void)materialOverride;
        return false;
    }
    virtual int doContainmentTest(const Vector3Dd &point, double distanceTolerance) { (void)point; (void)distanceTolerance; return OUTSIDE; }
    virtual void doExtraInformation(const RayWithSegments &ray, double t, PovRayHit *hit);
    virtual AxisAlignedBoundingBox getMinMax() const { return AxisAlignedBoundingBox::unbounded(); }
    virtual void *copy() = 0;
    virtual ~Geometry() {}

    virtual void invertGeometry() {}

  protected:
    // Implementation detail of doExtraInformation (its only caller, plus
    // intra-package uses): not part of the public surface, matching VITRAL's
    // Geometry, which has no public normal() at all.
    virtual void computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint) {}
    virtual void computeSurfaceNormal(
        Vector3Dd *result,
        Vector3Dd *intersectionPoint,
        const PovRayRendererConfiguration *config)
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
