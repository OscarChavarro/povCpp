#ifndef __BOUNDED_GEOMETRY_FACTORY__
#define __BOUNDED_GEOMETRY_FACTORY__

#include "environment/geometry/BoundedGeometry.h"

class PovrayMaterial;

class BoundedGeometryFactory {
  public:
    static BoundedGeometry *getObject(PovrayMaterial *defaultTexture);
    static BoundedGeometry *getObject(TransformableElement *geometry, PovrayMaterial *defaultTexture);
};

#endif
