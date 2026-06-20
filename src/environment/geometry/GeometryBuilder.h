#ifndef __GEOMETRY_BUILDER__
#define __GEOMETRY_BUILDER__

#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"

class GeometryBuilder {
  public:
    static PolynomialShape *getPolyShape(int order, const int *termCounts);
    static CSG *getCsgUnion();
    static CSG *getCsgIntersection();
};

#endif
