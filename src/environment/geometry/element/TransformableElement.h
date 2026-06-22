#ifndef __TRANSFORMABLE_ELEMENT__
#define __TRANSFORMABLE_ELEMENT__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/util/PriorityQueue.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/RendererConfiguration.h"

class IntersectionCandidate;

class TransformableElement {
  public:
    virtual int   allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue) { return 0; }
    bool doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out);
    virtual int   doContainmentTest(Vector3Dd *point) { return 0; }
    virtual void  normal(Vector3Dd *result, Vector3Dd *intersectionPoint) {}
    virtual void  normal(
        Vector3Dd *result,
        Vector3Dd *intersectionPoint,
        const RenderingConfiguration *config)
    {
        (void)config;
        normal(result, intersectionPoint);
    }
    virtual void *copy() = 0;
    virtual void  translate(Vector3Dd *vector) {}
    virtual void  rotate(Vector3Dd *vector) {}
    virtual void  scale(Vector3Dd *vector) {}
    virtual void  invert() {}
    virtual ~TransformableElement() {}
};

#endif
