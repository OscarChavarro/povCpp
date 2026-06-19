#ifndef __BOUNDED_GEOMETRY_FACTORY__
#define __BOUNDED_GEOMETRY_FACTORY__

#include "environment/geometry/BoundedGeometry.h"

class BoundedGeometryFactory {
  public:
    static BoundedGeometry *getObject(void);
    static BoundedGeometry *getObject(TransformableElement *geometry);
};

#endif
