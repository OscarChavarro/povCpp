#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "environment/geometry/element/TransformableElement.h"
#include "environment/geometry/GeometryConstants.h"

class SimpleBody;

// Pure geometric element: intersection math only. PovrayMaterial, colour and the
// transform/material bookkeeping now live on SimpleBody (scene layer).
//
// The *Geometry virtuals are the geometry-only half of the transform contract:
// they bake the transform into geometric parameters without touching any
// material. SimpleBody owns the full translate/rotate/scale (geometry +
// material + matrix accumulation) and delegates the geometry half here.
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
