#ifndef __TRANSFORMABLE_ELEMENT_H__
#define __TRANSFORMABLE_ELEMENT_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/util/PriorityQueue.h"
#include "environment/geometry/element/RayWithSegments.h"

class Intersection;

class TransformableElement {
  public:
    virtual int   allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) { return 0; }
    virtual int   inside(Vector3Dd *point) { return 0; }
    virtual void  normal(Vector3Dd *result, Vector3Dd *intersectionPoint) {}
    virtual void *copy() { return nullptr; }
    virtual void  translate(Vector3Dd *vector) {}
    virtual void  rotate(Vector3Dd *vector) {}
    virtual void  scale(Vector3Dd *vector) {}
    virtual void  invert() {}
    virtual ~TransformableElement() = default;
};

#endif
