#ifndef __GEOMETRY__
#define __GEOMETRY__

#include "environment/geometry/element/TransformableElement.h"

class SimpleBody;

class Geometry : public TransformableElement {
  public:
    virtual int allIntersectionsForOwner(
        RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue,
        SimpleBody *owner);
    virtual void translateGeometry(Vector3Dd *vector) {}
    virtual void rotateGeometry(Vector3Dd *vector) {}
    virtual void scaleGeometry(Vector3Dd *vector) {}
    virtual void invertGeometry() {}
};

#endif
