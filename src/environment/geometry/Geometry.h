#ifndef __GEOMETRY__
#define __GEOMETRY__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/util/PriorityQueue.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/PovRayRendererConfiguration.h"
#include "environment/material/Material.h"

class IntersectionCandidate;
class PovRayHit;

class Geometry {
  public:
    static constexpr int INSIDE = 1;
    static constexpr int LIMIT = 0;
    static constexpr int OUTSIDE = -1;

    virtual int allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue) { return 0; }
    virtual Geometry *getWrappedGeometry() const { return nullptr; }
    bool doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out);
    virtual int doContainmentTest(const Vector3Dd &point, double distanceTolerance) { (void)point; (void)distanceTolerance; return OUTSIDE; }
    virtual void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) {}
    virtual void normal(
        Vector3Dd *result,
        Vector3Dd *intersectionPoint,
        const PovRayRendererConfiguration *config)
    {
        (void)config;
        normal(result, intersectionPoint);
    }
    virtual void doExtraInformation(const RayWithSegments &ray, double t, PovRayHit *hit);
    virtual void *copy() = 0;
    virtual ~Geometry() {}

    virtual int allIntersectionsForMaterial(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *material);
    virtual void invertGeometry() {}
};

#endif
