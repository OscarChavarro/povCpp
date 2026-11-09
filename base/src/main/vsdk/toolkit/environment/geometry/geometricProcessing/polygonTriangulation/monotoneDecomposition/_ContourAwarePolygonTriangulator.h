#ifndef __CONTOUR_AWARE_POLYGON_TRIANGULATOR__
#define __CONTOUR_AWARE_POLYGON_TRIANGULATOR__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/Polygon2D.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/MonotoneDecompositionTriangulator.h"
class _ContourAwarePolygonTriangulator {
  public:
    static int triangulate(
        const Polygon2D &input,
        java::ArrayList<MonotoneDecompositionTriangulator::Triangle> &output);
};

#endif
