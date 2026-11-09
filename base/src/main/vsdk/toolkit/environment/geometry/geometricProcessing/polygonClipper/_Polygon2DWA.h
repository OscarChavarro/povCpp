#ifndef __POLYGON_2_DWA__
#define __POLYGON_2_DWA__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/Polygon2D.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/_Polygon2DContour.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/_Polygon2DContourWA.h"
class _Polygon2DWA {
public:
    java::ArrayList<_Polygon2DContourWA*> loops;

private:
    _Polygon2DContourWA* currentLoop;

    bool areCollinearAndOposite2DVectors(const Vertex2D& vEndA, const Vertex2D& vStartAB, const Vertex2D& vEndB);

public:
    _Polygon2DWA();
    _Polygon2DWA(const Polygon2D& polyToCopy, bool copyClean);
    ~_Polygon2DWA();

    void addVertex(double x, double y, double r, double g, double b);
    void addVertex(double x, double y);
    void pushVertex(double x, double y);
    void nextLoop();
};

#endif
