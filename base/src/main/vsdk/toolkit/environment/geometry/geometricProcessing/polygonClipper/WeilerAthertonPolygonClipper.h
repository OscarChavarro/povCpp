#ifndef __WEILER_ATHERTON_POLYGON_CLIPPER__
#define __WEILER_ATHERTON_POLYGON_CLIPPER__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/Polygon2D.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/PolygonTopologicalMerger.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/_CircularDoubleLinkedList.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/_DoubleLinkedListNode.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/_Polygon2DContourWA.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/_Polygon2DWA.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/_VertexNode2D.h"
class WeilerAthertonPolygonClipper {
private:
    _Polygon2DWA* clipPolyWA;
    _Polygon2DWA* subjectPolyWA;
    bool firstIntersection;
    bool previousOut;
    bool coincidentPoints[4];
    PolygonTopologicalMerger topologicalMerger;

    void makeCut(
        _Polygon2DContourWA* p2DContClip, _Polygon2DContourWA* p2DContSubj,
        _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodePrevC, _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeC,
        _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodePrevS, _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeS,
        _VertexNode2D& nodeIntersecS,
        java::ArrayList<_DoubleLinkedListNode<_VertexNode2D>*>& intersecVertListOut,
        java::ArrayList<_DoubleLinkedListNode<_VertexNode2D>*>& intersecVertListIn);

    void updatePolygonsAndListsWithCuts(
        _Polygon2DContourWA* p2DContClip, _Polygon2DContourWA* p2DContSubj,
        _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodePrevC, _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeC,
        _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodePrevS, _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeS,
        _VertexNode2D& nodeIntersecS, bool firstCutOutOfSubject,
        java::ArrayList<_DoubleLinkedListNode<_VertexNode2D>*>& intersecVertListOut,
        java::ArrayList<_DoubleLinkedListNode<_VertexNode2D>*>& intersecVertListIn,
        bool operateOnFirstPointsOfLines);

    void classifyHolesAndContours(_Polygon2DWA* polygon);
    signed char are3VectorsOrderedCounterclockwise2D(_VertexNode2D v1, _VertexNode2D v2, _VertexNode2D v3);
    signed char isPointInPolygon2D(const _VertexNode2D& point, _CircularDoubleLinkedList<_VertexNode2D>& polygon);
    double crossProduct2D(const _VertexNode2D& a1, const _VertexNode2D& a2, const _VertexNode2D& b1, const _VertexNode2D& b2);
    double crossProduct2D(const _VertexNode2D& v1, const _VertexNode2D& v2);
    double dotProduct2D(const _VertexNode2D& v1, const _VertexNode2D& v2);
    double dotProductNorm2D(const _VertexNode2D& a1, const _VertexNode2D& a2, const _VertexNode2D& b1, const _VertexNode2D& b2);
    _DoubleLinkedListNode<_VertexNode2D>* insertOrderedNodeBetweenTwoNodes(
        _CircularDoubleLinkedList<_VertexNode2D>& linkedList,
        _DoubleLinkedListNode<_VertexNode2D>* dllnVertNode, _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeNext,
        const _VertexNode2D& nodeIntersec);
    bool intersecLineLine2D(
        const _VertexNode2D& inP0, const _VertexNode2D& inP1, const _VertexNode2D& inP2, const _VertexNode2D& inP3,
        _VertexNode2D& outPIntersec, bool outCoincidentPoints[4]);

    static void resetOutputPolygon(Polygon2D* polygon);
    static void appendNonEmptyContours(const Polygon2D* source, Polygon2D* target);
    static bool hasAnyVertex(const Polygon2D* polygon);

public:
    WeilerAthertonPolygonClipper();
    ~WeilerAthertonPolygonClipper();
    void unionPolygons(Polygon2D* polygonA, Polygon2D* polygonB, Polygon2D* unionPolyOut);
    void clipPolygons(Polygon2D* clipPoly, Polygon2D* subjectPoly, Polygon2D* innerPolyOut, Polygon2D* outerPolyOut);
    _Polygon2DWA* getClipPolyWA();
    _Polygon2DWA* getSubjectPolyWA();
};

#endif
