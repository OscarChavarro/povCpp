#ifndef __GEOMETRY_BUILDER__
#define __GEOMETRY_BUILDER__

#include "environment/scene/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/PolynomialSurface.h"

class GeometryBuilder {
  public:
    static PolynomialSurface *getPolyShape(int order);
    static ConstructiveSolidGeometry *getCsgUnion();
    static ConstructiveSolidGeometry *getCsgIntersection();
};

#endif
