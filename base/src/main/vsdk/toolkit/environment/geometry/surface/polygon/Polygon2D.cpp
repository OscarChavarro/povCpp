#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/dataStructures/BinaryTreeNode.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex2D.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/Polygon2D.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/_Polygon2DContour.h"
class PolygonProcessor {
public:
    static void classifyContourHoles(Polygon2D* polygon);
};

Polygon2D::Polygon2D() : currentLoop(nullptr), headNode(nullptr) { nextLoop(); }

Polygon2D::~Polygon2D()
{
    for (long int i = 0; i < loops.size(); i++) delete loops[i];
}

void Polygon2D::addVertex(double x, double y, double r, double g, double b) { currentLoop->addVertex(x,y,r,g,b); }
void Polygon2D::addVertex(double x, double y) { currentLoop->addVertex(x,y); }
void Polygon2D::pushVertex(double x, double y) { currentLoop->pushVertex(x,y); }

void Polygon2D::nextLoop()
{
    currentLoop = new _Polygon2DContour();
    loops.add(currentLoop);
}

void Polygon2D::eraseLastLoop()
{
    if (loops.size() <= 1) return;
    _Polygon2DContour* last = loops[loops.size() - 1];
    for (long int i = 0; i < loops.size(); ) {
        if (loops[i]->getExteriorContour() == last) {
            delete loops[i];
            loops.remove(i);
        }
        else i++;
    }
    if (loops.size() > 1) {
        delete loops[loops.size() - 1];
        loops.remove(loops.size() - 1);
        currentLoop = loops[loops.size() - 1];
    }
    PolygonProcessor::classifyContourHoles(this);
}

void Polygon2D::invert()
{
    for (long int i = 0; i < loops.size(); i++) {
        java::ArrayList<Vertex2D>& verts = loops[i]->vertices;
        long int _n = verts.size();
        for (long int _i = 0; _i < _n / 2; _i++) {
            auto _tmp = verts[_i]; verts[_i] = verts[_n - 1 - _i]; verts[_n - 1 - _i] = _tmp;
        }
    }
}

double* Polygon2D::getMinMax()
{
    double* minMax = new double[6];
    double minX = 1e308, minY = 1e308;
    double maxX = -1e308, maxY = -1e308;

    for (long int i = 0; i < loops.size(); i++) {
        for (long int j = 0; j + 1 < loops[i]->vertices.size(); j++) {
            const Vertex2D& v = loops[i]->vertices[j];
            if (v.x > maxX) maxX = v.x;
            if (v.x < minX) minX = v.x;
            if (v.y > maxY) maxY = v.y;
            if (v.y < minY) minY = v.y;
        }
    }

    minMax[0]=minX; minMax[1]=minY; minMax[2]=0;
    minMax[3]=maxX; minMax[4]=maxY; minMax[5]=0;
    return minMax;
}

static bool pointInLoop(const _Polygon2DContour* loop, double x, double y)
{
    bool inside = false;
    java::ArrayList<Vertex2D>& v = const_cast<_Polygon2DContour*>(loop)->vertices;
    long int n = v.size();
    if (n < 3) return false;
    for (long int i = 0, j = n - 1; i < n; j = i++) {
        bool inter = ((v[i].y > y) != (v[j].y > y)) &&
                     (x < (v[j].x - v[i].x) * (y - v[i].y) / ((v[j].y - v[i].y) + 1e-30) + v[i].x);
        if (inter) inside = !inside;
    }
    return inside;
}

Ray* Polygon2D::doIntersectionFirstHit(const Ray& inOut_ray)
{
    RayHit hit;
    if (doIntersectionFirstHit(inOut_ray, &hit) && hit.getRay() != nullptr) {
        return new Ray(*hit.getRay());
    }
    return nullptr;
}

bool Polygon2D::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)
{
    if (std::abs(inRay.getDirection().z()) < VSDK::EPSILON) return false;
    double t = -inRay.getOrigin().z() / inRay.getDirection().z();
    if (t < 0) return false;

    Vector3Dd p = inRay.getOrigin().add(inRay.getDirection().multiply(t));
    bool inside = false;
    for (long int i = 0; i < loops.size(); i++) {
        if (pointInLoop(loops[i], p.x(), p.y())) {
            if (loops[i]->getExteriorContour() == nullptr) inside = true;
            else inside = false;
        }
    }
    if (!inside) return false;

    if (outHit != nullptr) {
        if (outHit->shouldStoreRay() || outHit->needsAnySurfaceData()) outHit->setRay(inRay.withT(t));
        else outHit->setHitDistance(t);
        if (outHit->needsPoint()) outHit->point = p;
        if (outHit->needsNormal()) outHit->normal = Vector3Dd(0,0,1);
    }
    return true;
}

void Polygon2D::doExtraInformation(const Ray& inRay, double intT, RayHit* outData)
{
    if (outData == nullptr) return;
    outData->point = inRay.getOrigin().add(inRay.getDirection().multiply(intT));
    outData->normal = Vector3Dd(0,0,1);
}

BinaryTreeNode<_Polygon2DContour*>* Polygon2D::getHeadNode() { return headNode; }
void Polygon2D::setHeadNode(BinaryTreeNode<_Polygon2DContour*>* node) { headNode = node; }
