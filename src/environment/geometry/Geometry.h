#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "environment/TransformableElement.h"
#include "environment/geometry/GeometryConstants.h"

// Pure geometric element: intersection math only. Material, colour and the
// transform/material bookkeeping now live on TranslatedBody (scene layer).
//
// The *Geometry virtuals are the geometry-only half of the transform contract:
// they bake the transform into geometric parameters without touching any
// material. TranslatedBody owns the full translate/rotate/scale (geometry +
// material + matrix accumulation) and delegates the geometry half here.
class Geometry : public TransformableElement {
  public:
    virtual void translateGeometry(Vector3Dd *vector) {}
    virtual void rotateGeometry(Vector3Dd *vector) {}
    virtual void scaleGeometry(Vector3Dd *vector) {}
    virtual void invertGeometry() {}
};

#endif
