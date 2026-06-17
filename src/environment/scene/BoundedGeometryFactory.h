#ifndef __BOUNDED_GEOMETRY_FACTORY_H__
#define __BOUNDED_GEOMETRY_FACTORY_H__

#include "environment/geometry/BoundedGeometry.h"

class BoundedGeometryFactory {
  public:
    static BoundedGeometry *getObject(void);
};

#endif
