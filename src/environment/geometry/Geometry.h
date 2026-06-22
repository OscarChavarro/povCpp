#ifndef __GEOMETRY__
#define __GEOMETRY__

#include "environment/geometry/element/TransformableElement.h"
#include "environment/material/Material.h"

class Geometry : public TransformableElement {
  public:
    virtual int allIntersectionsForMaterial(
        RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue,
        Material *material);
    virtual void translateGeometry(Vector3Dd *vector) {}
    virtual void rotateGeometry(Vector3Dd *vector) {}
    virtual void scaleGeometry(Vector3Dd *vector) {}
    virtual void invertGeometry() {}
};

#endif
