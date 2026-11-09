#ifndef __POLYGON_PROCESSOR__
#define __POLYGON_PROCESSOR__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/Polygon2D.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/_Polygon2DContour.h"
class PolygonProcessor {
public:
    static Polygon2D* polygon2DSimplify(const Polygon2D& pol2DIn, double epsilon, bool copy);
    static bool contourInsidePolygon(const java::ArrayList<Vertex2D>& contour, const java::ArrayList<Vertex2D>& mainPolygon);
    static signed char isPointInsidePolygon2D(const Vertex2D& point, const java::ArrayList<Vertex2D>& polygon);
    static void classifyContourHoles(Polygon2D* polygon);
};

#endif
