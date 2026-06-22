#ifndef __TRANSFORMABLE_ELEMENT__
#define __TRANSFORMABLE_ELEMENT__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/util/PriorityQueue.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/RendererConfiguration.h"

class IntersectionCandidate;
class PovRayHit;

class TransformableElement {
  public:
    static constexpr int INSIDE = 1;
    static constexpr int LIMIT = 0;
    static constexpr int OUTSIDE = -1;

    virtual int   allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue) { return 0; }
    bool doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out);
    virtual int   doContainmentTest(const Vector3Dd &point, double distanceTolerance) { (void)point; (void)distanceTolerance; return OUTSIDE; }
    virtual void  normal(Vector3Dd *result, Vector3Dd *intersectionPoint) {}
    virtual void  normal(
        Vector3Dd *result,
        Vector3Dd *intersectionPoint,
        const RenderingConfiguration *config)
    {
        (void)config;
        normal(result, intersectionPoint);
    }
    // Mirrors VITRAL's Geometry::doExtraInformation(const Ray&, double, RayHit*):
    // the entry point that fills in surface detail (here, just the normal) for
    // a resolved hit. hit->p must already hold the intersection point (it is
    // not re-derived from ray+t here, since some shapes - e.g.
    // ParametricBiCubicPatch - identify the cached normal by exact point
    // match against the value computed during allIntersections). The default
    // forwards to the existing normal() virtual so every shape gets this for
    // free without re-implementing anything; defined out-of-line in
    // TransformableElement.cpp to avoid a header cycle with PovRayHit.h
    // (PovRayHit.h -> Geometry.h -> TransformableElement.h).
    virtual void doExtraInformation(const RayWithSegments &ray, double t, PovRayHit *hit);
    virtual void *copy() = 0;
    virtual void  translate(Vector3Dd *vector) {}
    virtual void  rotate(Vector3Dd *vector) {}
    virtual void  scale(Vector3Dd *vector) {}
    virtual void  invert() {}
    virtual ~TransformableElement() {}
};

#endif
