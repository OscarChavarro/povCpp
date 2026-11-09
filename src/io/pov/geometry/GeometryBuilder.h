#ifndef __GEOMETRY_BUILDER__
#define __GEOMETRY_BUILDER__

#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/polynomial/PolynomialShape.h"

class GeometryBuilder {
  public:
    static PolynomialShape *getPolyShape(int order, const int *termCounts);
    static ConstructiveSolidGeometry *getCsgUnion();
    static ConstructiveSolidGeometry *getCsgIntersection();
};

#endif
