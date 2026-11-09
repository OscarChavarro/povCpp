#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/_Polygon2DWA.h"
_Polygon2DWA::_Polygon2DWA() : currentLoop(0) { nextLoop(); }

_Polygon2DWA::_Polygon2DWA(const Polygon2D& polyToCopy, bool copyClean) : currentLoop(0)
{
    bool oneOrMoreLoops = false;
    for (long int i = 0; i < polyToCopy.loops.size(); ++i) {
        _Polygon2DContour* contourToCopy = polyToCopy.loops.get(i);
        oneOrMoreLoops = true;
        nextLoop();
        if (contourToCopy->vertices.size() == 0) continue;
        if (contourToCopy->vertices.size() == 1) {
            Vertex2D prevV = contourToCopy->vertices.get(0);
            addVertex(prevV.x, prevV.y, prevV.color.r(), prevV.color.g(), prevV.color.b());
        }
        else {
            Vertex2D prevV = contourToCopy->vertices.get(contourToCopy->vertices.size()-1);
            Vertex2D prevPV = contourToCopy->vertices.get(contourToCopy->vertices.size()-2);
            bool removeLast = false;
            bool isFirst = true;
            for (long int j = 0; j < contourToCopy->vertices.size(); ++j) {
                Vertex2D v = contourToCopy->vertices.get(j);
                if (copyClean) {
                    if (std::fabs(prevV.x-v.x) > 0.0001 || std::fabs(prevV.y-v.y) > 0.0001) {
                        addVertex(v.x, v.y, v.color.r(), v.color.g(), v.color.b());
                        if (areCollinearAndOposite2DVectors(prevPV, prevV, v)) {
                            if (isFirst) removeLast = true;
                            else currentLoop->removeVertex(currentLoop->vertices.size()-2);
                        }
                        else prevPV = prevV;
                        prevV = v;
                    }
                    isFirst = false;
                }
                else addVertex(v.x, v.y, v.color.r(), v.color.g(), v.color.b());
            }
            if (removeLast) currentLoop->removeVertex(currentLoop->vertices.size()-1);
            if (currentLoop->vertices.size() == 0) {
                Vertex2D p = contourToCopy->vertices.get(0);
                addVertex(p.x, p.y, p.color.r(), p.color.g(), p.color.b());
            }
        }
    }
    if (!oneOrMoreLoops) nextLoop();
}

_Polygon2DWA::~_Polygon2DWA()
{
    for (long int i = 0; i < loops.size(); ++i) delete loops[i];
}

bool _Polygon2DWA::areCollinearAndOposite2DVectors(const Vertex2D& vEndA, const Vertex2D& vStartAB, const Vertex2D& vEndB)
{
    Vertex2D a(vEndA.x - vStartAB.x, vEndA.y - vStartAB.y);
    Vertex2D b(vEndB.x - vStartAB.x, vEndB.y - vStartAB.y);
    double t = std::sqrt(a.x*a.x + a.y*a.y);
    if (t < 1e-12) return false;
    a.x /= t; a.y /= t;
    t = std::sqrt(b.x*b.x + b.y*b.y);
    if (t < 1e-12) return false;
    b.x /= t; b.y /= t;
    t = a.x*b.x + a.y*b.y;
    return (t > -1.0001 && t < -0.9999);
}

void _Polygon2DWA::addVertex(double x, double y, double r, double g, double b) { currentLoop->addVertex(x,y,r,g,b); }
void _Polygon2DWA::addVertex(double x, double y) { currentLoop->addVertex(x,y); }
void _Polygon2DWA::pushVertex(double x, double y) { currentLoop->pushVertex(x,y); }
void _Polygon2DWA::nextLoop() { currentLoop = new _Polygon2DContourWA(); loops.add(currentLoop); }
