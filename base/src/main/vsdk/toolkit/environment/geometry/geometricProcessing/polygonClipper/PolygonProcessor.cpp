#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/dataStructures/BinaryTreeNode.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/PolygonProcessor.h"
static bool pointOnSegment(const Vertex2D& p, const Vertex2D& a, const Vertex2D& b)
{
    double cross = (p.y-a.y)*(b.x-a.x) - (p.x-a.x)*(b.y-a.y);
    if (std::fabs(cross) > 1e-12) return false;
    double dot = (p.x-a.x)*(b.x-a.x) + (p.y-a.y)*(b.y-a.y);
    if (dot < 0) return false;
    double len2 = (b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y);
    return dot <= len2;
}

Polygon2D* PolygonProcessor::polygon2DSimplify(const Polygon2D& pol2DIn, double epsilon, bool copy)
{
    (void)epsilon;
    Polygon2D* out = new Polygon2D();
    for (long int i = 0; i < out->loops.size(); ++i) delete out->loops[i];
    out->loops.clear();
    for (long int i = 0; i < pol2DIn.loops.size(); ++i) {
        _Polygon2DContour* src = pol2DIn.loops.get(i);
        _Polygon2DContour* dst = new _Polygon2DContour();
        for (long int j = 0; j < src->vertices.size(); ++j) {
            Vertex2D p = src->vertices[j];
            if (copy) dst->vertices.add(Vertex2D(p.x,p.y,p.color.r(),p.color.g(),p.color.b()));
            else dst->vertices.add(p);
        }
        out->loops.add(dst);
    }
    return out;
}

signed char PolygonProcessor::isPointInsidePolygon2D(const Vertex2D& point, const java::ArrayList<Vertex2D>& polygon)
{
    bool inside = false;
    long int n = polygon.size();
    if (n < 3) return -1;
    for (long int i = 0, j = n - 1; i < n; j = i++) {
        Vertex2D vi = polygon.get(i);
        Vertex2D vj = polygon.get(j);
        if (pointOnSegment(point, vj, vi)) return 0;
        bool intersect = ((vi.y > point.y) != (vj.y > point.y)) &&
                         (point.x < (vj.x - vi.x) * (point.y - vi.y) / ((vj.y - vi.y) + 1e-30) + vi.x);
        if (intersect) inside = !inside;
    }
    return inside ? 1 : -1;
}

bool PolygonProcessor::contourInsidePolygon(const java::ArrayList<Vertex2D>& contour, const java::ArrayList<Vertex2D>& mainPolygon)
{
    for (long int i = 0; i < contour.size(); ++i) {
        signed char r = isPointInsidePolygon2D(contour.get(i), mainPolygon);
        if (r == 1) return true;
        if (r == -1) return false;
    }
    return false;
}

void PolygonProcessor::classifyContourHoles(Polygon2D* polygon)
{
    if (polygon == 0) return;
    for (long int i = 0; i < polygon->loops.size(); ++i) polygon->loops[i]->setExteriorContour(0);
    BinaryTreeNode<_Polygon2DContour*>* root = new BinaryTreeNode<_Polygon2DContour*>(0);
    polygon->setHeadNode(root);
    for (long int i = 0; i < polygon->loops.size(); ++i) {
        _Polygon2DContour* a = polygon->loops[i];
        for (long int j = 0; j < polygon->loops.size(); ++j) {
            if (i == j) continue;
            _Polygon2DContour* b = polygon->loops[j];
            if (contourInsidePolygon(a->vertices, b->vertices)) {
                a->setExteriorContour(b);
                break;
            }
        }
    }
}
