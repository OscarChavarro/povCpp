#ifndef __POLYGON_2_D_CONTOUR_WA__
#define __POLYGON_2_D_CONTOUR_WA__

#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/_CircularDoubleLinkedList.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/_VertexNode2D.h"
class _Polygon2DContourWA {
public:
    _CircularDoubleLinkedList<_VertexNode2D> vertices;
    bool isClipped;
    bool isHole;

    _Polygon2DContourWA() : isClipped(false), isHole(false) {}
    void addVertex(double x, double y, double r, double g, double b) { vertices.add(_VertexNode2D(x, y, r, g, b)); }
    void addVertex(double x, double y) { vertices.add(_VertexNode2D(x, y)); }
    void removeVertex(int ind) { vertices.remove(ind); }
    void pushVertex(double x, double y) { vertices.add(0, _VertexNode2D(x, y)); }
};

#endif
