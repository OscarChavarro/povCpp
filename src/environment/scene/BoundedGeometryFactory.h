#ifndef __BOUNDED_GEOMETRY_FACTORY__
#define __BOUNDED_GEOMETRY_FACTORY__

#include "environment/geometry/BoundedGeometry.h"

class PovRayMaterial;

class BoundedGeometryFactory {
  public:
    static BoundedGeometry *getBoundedGeometry(PovRayMaterial *defaultTexture);
    static BoundedGeometry *getBoundedGeometry(TransformableElement *geometry, PovRayMaterial *defaultTexture);
};

#endif
