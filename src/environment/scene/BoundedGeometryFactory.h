#ifndef __BOUNDED_GEOMETRY_FACTORY__
#define __BOUNDED_GEOMETRY_FACTORY__

#include "environment/geometry/BoundedGeometry.h"

class PovrayMaterial;

class BoundedGeometryFactory {
  public:
    static BoundedGeometry *getBoundedGeometry(PovrayMaterial *defaultTexture);
    static BoundedGeometry *getBoundedGeometry(TransformableElement *geometry, PovrayMaterial *defaultTexture);
};

#endif
