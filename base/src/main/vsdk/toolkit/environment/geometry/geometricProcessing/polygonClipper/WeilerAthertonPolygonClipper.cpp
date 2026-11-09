#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/WeilerAthertonPolygonClipper.h"
WeilerAthertonPolygonClipper::WeilerAthertonPolygonClipper()
    : clipPolyWA(0), subjectPolyWA(0), firstIntersection(false), previousOut(false)
{
    coincidentPoints[0] = coincidentPoints[1] = coincidentPoints[2] = coincidentPoints[3] = false;
}

WeilerAthertonPolygonClipper::~WeilerAthertonPolygonClipper()
{
    if (clipPolyWA) delete clipPolyWA;
    if (subjectPolyWA) delete subjectPolyWA;
}

void WeilerAthertonPolygonClipper::unionPolygons(Polygon2D* polygonA, Polygon2D* polygonB, Polygon2D* unionPolyOut)
{
    if (!polygonA || !polygonB || !unionPolyOut) return;
    Polygon2D aMinusB;
    Polygon2D bMinusA;
    Polygon2D intersectionAB;
    Polygon2D temp;

    clipPolygons(polygonB, polygonA, &intersectionAB, &aMinusB);
    clipPolygons(polygonA, polygonB, &temp, &bMinusA);

    resetOutputPolygon(unionPolyOut);
    appendNonEmptyContours(&aMinusB, unionPolyOut);
    appendNonEmptyContours(&bMinusA, unionPolyOut);
    appendNonEmptyContours(&intersectionAB, unionPolyOut);
    topologicalMerger.mergeInPlace(unionPolyOut);
}

void WeilerAthertonPolygonClipper::clipPolygons(Polygon2D* clipPoly, Polygon2D* subjectPoly, Polygon2D* innerPolyOut, Polygon2D* outerPolyOut)
{
    if (!innerPolyOut || !outerPolyOut || !clipPoly || !subjectPoly) return;

    if (clipPolyWA) { delete clipPolyWA; clipPolyWA = 0; }
    if (subjectPolyWA) { delete subjectPolyWA; subjectPolyWA = 0; }
    clipPolyWA = new _Polygon2DWA(*clipPoly, true);
    subjectPolyWA = new _Polygon2DWA(*subjectPoly, true);

    resetOutputPolygon(innerPolyOut);
    resetOutputPolygon(outerPolyOut);

    java::ArrayList<_DoubleLinkedListNode<_VertexNode2D>*> intersecVertListOut;
    java::ArrayList<_DoubleLinkedListNode<_VertexNode2D>*> intersecVertListIn;

    bool emptyInnerPolyOut = true;
    bool emptyOuterPolyOut = true;

    for (long int i = 0; i < clipPolyWA->loops.size(); ++i) {
        _Polygon2DContourWA* p2DContClip = clipPolyWA->loops.get(i);
        p2DContClip->isClipped = false;
        _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeC = p2DContClip->vertices.getHead();
        if (p2DContClip->vertices.size() > 1) {
            long int guardC = 0;
            do {
                if (++guardC > 5000) {
                    return;
                }
                _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodePrevC = dllnVertNodeC->previous;
                _VertexNode2D nodeC = dllnVertNodeC->data;
                _VertexNode2D nodePrevC = dllnVertNodePrevC->data;
                previousOut = false;

                for (long int j = 0; j < subjectPolyWA->loops.size(); ++j) {
                    _Polygon2DContourWA* p2DContSubj = subjectPolyWA->loops.get(j);
                    _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeS = p2DContSubj->vertices.getHead();
                    if (p2DContSubj->vertices.size() > 1) {
                        firstIntersection = true;
                        long int guardS = 0;
                        do {
                            if (++guardS > 5000) {
                                return;
                            }
                            _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodePrevS = dllnVertNodeS->previous;
                            _VertexNode2D nodeS = dllnVertNodeS->data;
                            _VertexNode2D nodePrevS = dllnVertNodePrevS->data;
                            _VertexNode2D nodeIntersecS;

                            if (intersecLineLine2D(nodePrevC, nodeC, nodePrevS, nodeS, nodeIntersecS, coincidentPoints)) {
                                makeCut(p2DContClip, p2DContSubj, dllnVertNodePrevC, dllnVertNodeC,
                                    dllnVertNodePrevS, dllnVertNodeS, nodeIntersecS,
                                    intersecVertListOut, intersecVertListIn);
                            }
                            dllnVertNodeS = dllnVertNodeS->next;
                        } while (dllnVertNodeS != p2DContSubj->vertices.getHead());
                    }
                }
                dllnVertNodeC = dllnVertNodeC->next;
            } while (dllnVertNodeC != p2DContClip->vertices.getHead());
        }
    }

    for (long int i = 0; i < intersecVertListOut.size(); ++i) {
        _DoubleLinkedListNode<_VertexNode2D>* dllnNodeS = intersecVertListOut.get(i);
        if ((dllnNodeS->data.flags & 0x01) == 0) {
            if (emptyInnerPolyOut) emptyInnerPolyOut = false;
            else innerPolyOut->nextLoop();

            _DoubleLinkedListNode<_VertexNode2D>* iterator = dllnNodeS;
            long int guardInner = 0;
            do {
                if (++guardInner > 5000) {
                    return;
                }
                iterator->data.flags = (unsigned char)(iterator->data.flags | 0x01);
                innerPolyOut->addVertex(iterator->data.x, iterator->data.y,
                    iterator->data.color.r(), iterator->data.color.g(), iterator->data.color.b());
                iterator = iterator->next;
                iterator->data.flags = (unsigned char)(iterator->data.flags | 0x01);
                if (iterator->data.pairNode != 0) iterator = iterator->data.pairNode;
            } while (iterator != dllnNodeS && iterator->data.pairNode != dllnNodeS);
        }
    }

    for (long int i = 0; i < intersecVertListIn.size(); ++i) {
        _DoubleLinkedListNode<_VertexNode2D>* dllnNodeS = intersecVertListIn.get(i);
        if ((dllnNodeS->data.flags & 0x02) == 0) {
            bool isSubject = true;
            if (emptyOuterPolyOut) emptyOuterPolyOut = false;
            else outerPolyOut->nextLoop();

            _DoubleLinkedListNode<_VertexNode2D>* iterator = dllnNodeS;
            long int guardOuter = 0;
            do {
                if (++guardOuter > 5000) {
                    return;
                }
                iterator->data.flags = (unsigned char)(iterator->data.flags | 0x02);
                outerPolyOut->addVertex(iterator->data.x, iterator->data.y,
                    iterator->data.color.r(), iterator->data.color.g(), iterator->data.color.b());
                iterator = isSubject ? iterator->next : iterator->previous;
                iterator->data.flags = (unsigned char)(iterator->data.flags | 0x02);
                if (iterator->data.pairNode != 0) {
                    iterator = iterator->data.pairNode;
                    isSubject = !isSubject;
                }
            } while (iterator != dllnNodeS && iterator->data.pairNode != dllnNodeS);
        }
    }

    classifyHolesAndContours(getClipPolyWA());
    classifyHolesAndContours(getSubjectPolyWA());

    for (long int i = 0; i < clipPolyWA->loops.size(); ++i) {
        _Polygon2DContourWA* p2DContClip = clipPolyWA->loops.get(i);
        if (!p2DContClip->isClipped) {
            _DoubleLinkedListNode<_VertexNode2D>* head = p2DContClip->vertices.getHead();
            if (!head) continue;
            bool insideAContourNotAHole = false;

            for (long int j = 0; j < subjectPolyWA->loops.size(); ++j) {
                _Polygon2DContourWA* p2DContSubj = subjectPolyWA->loops.get(j);
                bool pointInPolygon = false;
                _DoubleLinkedListNode<_VertexNode2D>* dllnNode = head;
                do {
                    if (isPointInPolygon2D(dllnNode->data, p2DContSubj->vertices) == 1) { pointInPolygon = true; break; }
                    dllnNode = dllnNode->next;
                } while (dllnNode != head);
                if (pointInPolygon) insideAContourNotAHole = !insideAContourNotAHole;
            }

            if (insideAContourNotAHole) {
                if (emptyInnerPolyOut) emptyInnerPolyOut = false; else innerPolyOut->nextLoop();
                if (emptyOuterPolyOut) emptyOuterPolyOut = false; else outerPolyOut->nextLoop();

                _DoubleLinkedListNode<_VertexNode2D>* dllnNode = head;
                do {
                    innerPolyOut->addVertex(dllnNode->data.x, dllnNode->data.y,
                        dllnNode->data.color.r(), dllnNode->data.color.g(), dllnNode->data.color.b());
                    dllnNode = dllnNode->next;
                } while (dllnNode != head);

                dllnNode = head;
                do {
                    outerPolyOut->addVertex(dllnNode->data.x, dllnNode->data.y,
                        dllnNode->data.color.r(), dllnNode->data.color.g(), dllnNode->data.color.b());
                    dllnNode = dllnNode->previous;
                } while (dllnNode != head);
            }
        }
    }

    for (long int i = 0; i < subjectPolyWA->loops.size(); ++i) {
        _Polygon2DContourWA* p2DContSubj = subjectPolyWA->loops.get(i);
        if (!p2DContSubj->isClipped) {
            _DoubleLinkedListNode<_VertexNode2D>* head = p2DContSubj->vertices.getHead();
            if (!head) continue;
            bool insideAContourNotAHole = false;

            for (long int j = 0; j < clipPolyWA->loops.size(); ++j) {
                _Polygon2DContourWA* p2DContClip = clipPolyWA->loops.get(j);
                bool pointInPolygon = false;
                _DoubleLinkedListNode<_VertexNode2D>* dllnNode = head;
                do {
                    if (isPointInPolygon2D(dllnNode->data, p2DContClip->vertices) == 1) { pointInPolygon = true; break; }
                    dllnNode = dllnNode->next;
                } while (dllnNode != head);
                if (pointInPolygon) insideAContourNotAHole = !insideAContourNotAHole;
            }

            if (insideAContourNotAHole) { if (emptyInnerPolyOut) emptyInnerPolyOut = false; else innerPolyOut->nextLoop(); }
            else { if (emptyOuterPolyOut) emptyOuterPolyOut = false; else outerPolyOut->nextLoop(); }

            _DoubleLinkedListNode<_VertexNode2D>* dllnNode = head;
            do {
                if (insideAContourNotAHole) {
                    innerPolyOut->addVertex(dllnNode->data.x, dllnNode->data.y,
                        dllnNode->data.color.r(), dllnNode->data.color.g(), dllnNode->data.color.b());
                }
                else {
                    outerPolyOut->addVertex(dllnNode->data.x, dllnNode->data.y,
                        dllnNode->data.color.r(), dllnNode->data.color.g(), dllnNode->data.color.b());
                }
                dllnNode = dllnNode->next;
            } while (dllnNode != head);
        }
    }
}

void WeilerAthertonPolygonClipper::makeCut(
    _Polygon2DContourWA* p2DContClip, _Polygon2DContourWA* p2DContSubj,
    _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodePrevC, _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeC,
    _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodePrevS, _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeS,
    _VertexNode2D& nodeIntersecS,
    java::ArrayList<_DoubleLinkedListNode<_VertexNode2D>*>& intersecVertListOut,
    java::ArrayList<_DoubleLinkedListNode<_VertexNode2D>*>& intersecVertListIn)
{
    _VertexNode2D nodeC;
    _VertexNode2D nodePrevC;
    _VertexNode2D nodeS;
    _VertexNode2D nodePrevS;
    double dotProd;
    bool firstCutOutOfSubject;

    if (!coincidentPoints[0] && !coincidentPoints[2]) {
        firstCutOutOfSubject = false;
        if (firstIntersection) {
            firstCutOutOfSubject = (crossProduct2D(
                dllnVertNodePrevS->data, dllnVertNodeS->data, dllnVertNodePrevC->data, dllnVertNodeC->data) < 0);
        }
        updatePolygonsAndListsWithCuts(
            p2DContClip, p2DContSubj, dllnVertNodePrevC, dllnVertNodeC, dllnVertNodePrevS, dllnVertNodeS, nodeIntersecS,
            firstCutOutOfSubject, intersecVertListOut, intersecVertListIn, true);
        return;
    }
    nodePrevC = dllnVertNodePrevC->data;
    nodeC = dllnVertNodeC->data;
    nodePrevS = dllnVertNodePrevS->data;
    nodeS = dllnVertNodeS->data;
    dotProd = dotProductNorm2D(nodePrevC, nodeC, nodePrevS, nodeS);
    if (dotProd >= 0.9999 && dotProd <= 1.0001) {
        _VertexNode2D vecParallel;
        _VertexNode2D negVecParallel;
        _VertexNode2D vecAwayParallelLines1C;
        _VertexNode2D vecAwayParallelLines1S;
        _VertexNode2D vecAwayParallelLines2C;
        _VertexNode2D vecAwayParallelLines2S;

        vecParallel.x = nodeC.x - nodePrevC.x;
        vecParallel.y = nodeC.y - nodePrevC.y;
        negVecParallel.x = -vecParallel.x;
        negVecParallel.y = -vecParallel.y;

        if (coincidentPoints[0]) {
            vecAwayParallelLines1C.x = dllnVertNodePrevC->previous->data.x - dllnVertNodePrevC->data.x;
            vecAwayParallelLines1C.y = dllnVertNodePrevC->previous->data.y - dllnVertNodePrevC->data.y;
        } else { vecAwayParallelLines1C.x = negVecParallel.x; vecAwayParallelLines1C.y = negVecParallel.y; }

        if (coincidentPoints[2]) {
            vecAwayParallelLines1S.x = dllnVertNodePrevS->previous->data.x - dllnVertNodePrevS->data.x;
            vecAwayParallelLines1S.y = dllnVertNodePrevS->previous->data.y - dllnVertNodePrevS->data.y;
        } else { vecAwayParallelLines1S.x = negVecParallel.x; vecAwayParallelLines1S.y = negVecParallel.y; }

        if (coincidentPoints[1]) {
            vecAwayParallelLines2C.x = dllnVertNodeC->next->data.x - dllnVertNodeC->data.x;
            vecAwayParallelLines2C.y = dllnVertNodeC->next->data.y - dllnVertNodeC->data.y;
        } else { vecAwayParallelLines2C.x = vecParallel.x; vecAwayParallelLines2C.y = vecParallel.y; }

        if (coincidentPoints[3]) {
            vecAwayParallelLines2S.x = dllnVertNodeS->next->data.x - dllnVertNodeS->data.x;
            vecAwayParallelLines2S.y = dllnVertNodeS->next->data.y - dllnVertNodeS->data.y;
        } else { vecAwayParallelLines2S.x = vecParallel.x; vecAwayParallelLines2S.y = vecParallel.y; }

        if (are3VectorsOrderedCounterclockwise2D(
            vecParallel, vecAwayParallelLines1C, vecAwayParallelLines1S) == 1) {
            firstCutOutOfSubject = true;
            updatePolygonsAndListsWithCuts(p2DContClip, p2DContSubj,
                dllnVertNodePrevC, dllnVertNodeC, dllnVertNodePrevS, dllnVertNodeS,
                nodeIntersecS, true, intersecVertListOut, intersecVertListIn, true);
        }
        if (are3VectorsOrderedCounterclockwise2D(
            negVecParallel, vecAwayParallelLines2S, vecAwayParallelLines2C) == 1) {
            firstCutOutOfSubject = false;
            updatePolygonsAndListsWithCuts(p2DContClip, p2DContSubj,
                dllnVertNodePrevC, dllnVertNodeC, dllnVertNodePrevS, dllnVertNodeS,
                nodeIntersecS, false, intersecVertListOut, intersecVertListIn, false);
        }
    }
    else if (std::fabs(dotProd) < 0.9999) {
        _VertexNode2D vecAC;
        _VertexNode2D vecBC;
        _VertexNode2D vecAS;
        _VertexNode2D vecBS;
        bool thereAreCut;
        signed char orderVecAC, orderVecBC;

        vecAC.x = dllnVertNodeC->data.x - dllnVertNodePrevC->data.x;
        vecAC.y = dllnVertNodeC->data.y - dllnVertNodePrevC->data.y;
        if (coincidentPoints[0]) {
            vecBC.x = dllnVertNodePrevC->previous->data.x - dllnVertNodePrevC->data.x;
            vecBC.y = dllnVertNodePrevC->previous->data.y - dllnVertNodePrevC->data.y;
        }
        else {
            vecBC.x = -vecAC.x;
            vecBC.y = -vecAC.y;
        }
        vecAS.x = dllnVertNodeS->data.x - dllnVertNodePrevS->data.x;
        vecAS.y = dllnVertNodeS->data.y - dllnVertNodePrevS->data.y;
        if (coincidentPoints[2]) {
            vecBS.x = dllnVertNodePrevS->previous->data.x - dllnVertNodePrevS->data.x;
            vecBS.y = dllnVertNodePrevS->previous->data.y - dllnVertNodePrevS->data.y;
        }
        else {
            vecBS.x = -vecAS.x;
            vecBS.y = -vecAS.y;
        }
        orderVecAC = are3VectorsOrderedCounterclockwise2D(vecAS, vecAC, vecBS);
        orderVecBC = are3VectorsOrderedCounterclockwise2D(vecAS, vecBC, vecBS);
        thereAreCut = false;
        firstCutOutOfSubject = false;
        if (orderVecAC != 0 && orderVecBC != 0) {
            if ((orderVecAC == 1) != (orderVecBC == 1)) {
                thereAreCut = true;
                firstCutOutOfSubject = (orderVecAC == -1);
            }
        }
        if (thereAreCut) {
            updatePolygonsAndListsWithCuts(
                p2DContClip, p2DContSubj, dllnVertNodePrevC, dllnVertNodeC, dllnVertNodePrevS, dllnVertNodeS, nodeIntersecS,
                firstCutOutOfSubject, intersecVertListOut, intersecVertListIn, true);
        }
    }
}

void WeilerAthertonPolygonClipper::updatePolygonsAndListsWithCuts(
    _Polygon2DContourWA* p2DContClip, _Polygon2DContourWA* p2DContSubj,
    _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodePrevC, _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeC,
    _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodePrevS, _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeS,
    _VertexNode2D& nodeIntersecS, bool firstCutOutOfSubject,
    java::ArrayList<_DoubleLinkedListNode<_VertexNode2D>*>& intersecVertListOut,
    java::ArrayList<_DoubleLinkedListNode<_VertexNode2D>*>& intersecVertListIn,
    bool operateOnFirstPointsOfLines)
{
    _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeCutS = 0;

    if (!coincidentPoints[0] && !coincidentPoints[2]) {
        _VertexNode2D nodeIntersecC(nodeIntersecS);
        _DoubleLinkedListNode<_VertexNode2D>* clipInserted =
            insertOrderedNodeBetweenTwoNodes(
                p2DContClip->vertices, dllnVertNodePrevC, dllnVertNodeC, nodeIntersecC);
        _DoubleLinkedListNode<_VertexNode2D>* subjInserted =
            p2DContSubj->vertices.insertBefore(nodeIntersecS, dllnVertNodeS);
        // IMPORTANT: in C++ list stores values, so pairing must be written back
        // to list nodes (local temporaries do not propagate like Java refs).
        clipInserted->data.pairNode = subjInserted;
        subjInserted->data.pairNode = clipInserted;
        dllnVertNodeCutS = subjInserted;
    }
    else {
        _VertexNode2D nodeIntersec;
        if (operateOnFirstPointsOfLines) {
            if (coincidentPoints[0]) {
                if (coincidentPoints[2]) {
                    dllnVertNodePrevC->data.pairNode = dllnVertNodePrevS;
                    dllnVertNodePrevS->data.pairNode = dllnVertNodePrevC;
                    dllnVertNodeCutS = dllnVertNodePrevS;
                }
                else {
                    nodeIntersec = _VertexNode2D(dllnVertNodePrevC->data);
                    _DoubleLinkedListNode<_VertexNode2D>* subjInserted =
                        p2DContSubj->vertices.insertBefore(nodeIntersec, dllnVertNodeS);
                    dllnVertNodePrevC->data.pairNode = subjInserted;
                    subjInserted->data.pairNode = dllnVertNodePrevC;
                    dllnVertNodeCutS = dllnVertNodePrevC->data.pairNode;
                }
            }
            else if (coincidentPoints[2]) {
                nodeIntersec = _VertexNode2D(dllnVertNodePrevS->data);
                _DoubleLinkedListNode<_VertexNode2D>* clipInserted = insertOrderedNodeBetweenTwoNodes(
                    p2DContClip->vertices, dllnVertNodePrevC, dllnVertNodeC, nodeIntersec);
                dllnVertNodePrevS->data.pairNode = clipInserted;
                clipInserted->data.pairNode = dllnVertNodePrevS;
                dllnVertNodeCutS = dllnVertNodePrevS;
            }
        }
        else {
            if (coincidentPoints[1]) {
                if (coincidentPoints[3]) {
                    dllnVertNodeC->data.pairNode = dllnVertNodeS;
                    dllnVertNodeS->data.pairNode = dllnVertNodeC;
                    dllnVertNodeCutS = dllnVertNodeS;
                }
                else {
                    nodeIntersec = _VertexNode2D(dllnVertNodeC->data);
                    _DoubleLinkedListNode<_VertexNode2D>* subjInserted =
                        p2DContSubj->vertices.insertBefore(nodeIntersec, dllnVertNodeS);
                    dllnVertNodeC->data.pairNode = subjInserted;
                    subjInserted->data.pairNode = dllnVertNodeC;
                    dllnVertNodeCutS = dllnVertNodeC->data.pairNode;
                }
            }
            else if (coincidentPoints[3]) {
                nodeIntersec = _VertexNode2D(dllnVertNodeS->data);
                _DoubleLinkedListNode<_VertexNode2D>* clipInserted = insertOrderedNodeBetweenTwoNodes(
                    p2DContClip->vertices, dllnVertNodePrevC, dllnVertNodeC, nodeIntersec);
                dllnVertNodeS->data.pairNode = clipInserted;
                clipInserted->data.pairNode = dllnVertNodeS;
                dllnVertNodeCutS = dllnVertNodeS;
            }
        }
    }

    if (firstIntersection) {
        p2DContSubj->isClipped = true;
        p2DContClip->isClipped = true;
        if (firstCutOutOfSubject) { intersecVertListOut.add(dllnVertNodeCutS); previousOut = true; }
        else { intersecVertListIn.add(dllnVertNodeCutS); previousOut = false; }
        firstIntersection = false;
    }
    else {
        if (previousOut) intersecVertListIn.add(dllnVertNodeCutS);
        else intersecVertListOut.add(dllnVertNodeCutS);
        previousOut = !previousOut;
    }
}

void WeilerAthertonPolygonClipper::classifyHolesAndContours(_Polygon2DWA* polygon)
{
    for (long int i = 0; i < polygon->loops.size(); ++i) {
        _Polygon2DContourWA* p2DContTest = polygon->loops.get(i);
        p2DContTest->isHole = false;
        for (long int j = 0; j < polygon->loops.size(); ++j) {
            _Polygon2DContourWA* p2DCont = polygon->loops.get(j);
            if (p2DCont != p2DContTest) {
                _DoubleLinkedListNode<_VertexNode2D>* head = p2DContTest->vertices.getHead();
                if (!head) continue;
                bool pointInPolygon = false;
                _DoubleLinkedListNode<_VertexNode2D>* dllnNode = head;
                do {
                    if (isPointInPolygon2D(dllnNode->data, p2DCont->vertices) == 1) {
                        pointInPolygon = true;
                        break;
                    }
                    dllnNode = dllnNode->next;
                } while (dllnNode != head);
                if (pointInPolygon) p2DContTest->isHole = !p2DContTest->isHole;
            }
        }
    }
}

signed char WeilerAthertonPolygonClipper::are3VectorsOrderedCounterclockwise2D(_VertexNode2D v1, _VertexNode2D v2, _VertexNode2D v3)
{
    double t = std::sqrt(v1.x*v1.x + v1.y*v1.y); if (t < 1e-12) return 0; v1.x/=t; v1.y/=t;
    t = std::sqrt(v2.x*v2.x + v2.y*v2.y); if (t < 1e-12) return 0; v2.x/=t; v2.y/=t;
    t = std::sqrt(v3.x*v3.x + v3.y*v3.y); if (t < 1e-12) return 0; v3.x/=t; v3.y/=t;

    t = dotProduct2D(v1, v2); if (t > 0.9999 && t < 1.0001) return 0;
    t = dotProduct2D(v2, v3); if (t > 0.9999 && t < 1.0001) return 0;
    t = dotProduct2D(v1, v3); if (t > 0.9999 && t < 1.0001) return 0;

    if (crossProduct2D(v1, v3) < 0) {
        if (crossProduct2D(v2, v3) >= 0) return 1;
        else if (crossProduct2D(v1, v2) > 0) return 1;
        else return -1;
    }
    else {
        if (crossProduct2D(v2, v3) <= 0) return -1;
        else if (crossProduct2D(v2, v1) > 0) return -1;
        else return 1;
    }
}

signed char WeilerAthertonPolygonClipper::isPointInPolygon2D(const _VertexNode2D& point, _CircularDoubleLinkedList<_VertexNode2D>& polygon)
{
    _DoubleLinkedListNode<_VertexNode2D>* dllnNode = polygon.getHead();
    if (!dllnNode) return -1;
    bool isInside = false;
    do {
        double temp = dllnNode->data.y - dllnNode->next->data.y;
        if (std::fabs(temp) < 0.0001) {
            if (std::fabs(point.y - (dllnNode->next->data.y + temp/2)) < 0.0001) {
                if ((dllnNode->next->data.x - point.x) * (point.x - dllnNode->data.x) >= 0) return 0;
            }
        }
        if (((point.y < dllnNode->data.y) != (point.y < dllnNode->next->data.y))) {
            temp = ((dllnNode->next->data.x - dllnNode->data.x) * (point.y - dllnNode->data.y) / (dllnNode->next->data.y - dllnNode->data.y) + dllnNode->data.x);
            if (std::fabs(point.x - temp) < 0.0001) return 0;
            if (point.x < temp) isInside = !isInside;
        }
        dllnNode = dllnNode->next;
    } while (dllnNode != polygon.getHead());
    return isInside ? 1 : -1;
}

double WeilerAthertonPolygonClipper::crossProduct2D(const _VertexNode2D& a1, const _VertexNode2D& a2, const _VertexNode2D& b1, const _VertexNode2D& b2)
{ return (a2.x-a1.x)*(b2.y-b1.y) - (a2.y-a1.y)*(b2.x-b1.x); }

double WeilerAthertonPolygonClipper::crossProduct2D(const _VertexNode2D& v1, const _VertexNode2D& v2)
{ return v1.x*v2.y - v1.y*v2.x; }

double WeilerAthertonPolygonClipper::dotProduct2D(const _VertexNode2D& v1, const _VertexNode2D& v2)
{ return v1.x*v2.x + v1.y*v2.y; }

double WeilerAthertonPolygonClipper::dotProductNorm2D(const _VertexNode2D& a1, const _VertexNode2D& a2, const _VertexNode2D& b1, const _VertexNode2D& b2)
{
    double v1x = a2.x-a1.x, v1y = a2.y-a1.y;
    double v2x = b2.x-b1.x, v2y = b2.y-b1.y;
    double t = std::sqrt(v1x*v1x + v1y*v1y); if (t < 1e-12) return 0; v1x/=t; v1y/=t;
    t = std::sqrt(v2x*v2x + v2y*v2y); if (t < 1e-12) return 0; v2x/=t; v2y/=t;
    return v1x*v2x + v1y*v2y;
}

_DoubleLinkedListNode<_VertexNode2D>* WeilerAthertonPolygonClipper::insertOrderedNodeBetweenTwoNodes(
    _CircularDoubleLinkedList<_VertexNode2D>& linkedList,
    _DoubleLinkedListNode<_VertexNode2D>* dllnVertNode, _DoubleLinkedListNode<_VertexNode2D>* dllnVertNodeNext,
    const _VertexNode2D& nodeIntersec)
{
    _DoubleLinkedListNode<_VertexNode2D>* dllnNode = dllnVertNode;
    if (std::fabs(dllnVertNode->data.x - dllnVertNodeNext->data.x) > std::fabs(dllnVertNode->data.y - dllnVertNodeNext->data.y)) {
        int sign = (int)((dllnVertNodeNext->data.x - dllnVertNode->data.x) / std::fabs(dllnVertNodeNext->data.x - dllnVertNode->data.x));
        while (dllnNode->data.x * sign < nodeIntersec.x * sign && dllnNode != dllnVertNodeNext) dllnNode = dllnNode->next;
    }
    else {
        int sign = (int)((dllnVertNodeNext->data.y - dllnVertNode->data.y) / std::fabs(dllnVertNodeNext->data.y - dllnVertNode->data.y));
        while (dllnNode->data.y * sign < nodeIntersec.y * sign && dllnNode != dllnVertNodeNext) dllnNode = dllnNode->next;
    }
    return linkedList.insertBefore(nodeIntersec, dllnNode);
}

bool WeilerAthertonPolygonClipper::intersecLineLine2D(
    const _VertexNode2D& inP0, const _VertexNode2D& inP1, const _VertexNode2D& inP2, const _VertexNode2D& inP3,
    _VertexNode2D& outPIntersec, bool outCoincidentPoints[4])
{
    double temp1, temp2;
    bool vertical1, vertical2;
    double m1, b1, m2, b2;
    bool intersec1, intersec2;
    bool pointToPointCoincidence;
    bool point0ToPoint2Coincidence;
    bool point0ToPoint3Coincidence;
    bool point2ToPoint1Coincidence;

    outCoincidentPoints[0] = false;
    outCoincidentPoints[1] = false;
    outCoincidentPoints[2] = false;
    outCoincidentPoints[3] = false;
    pointToPointCoincidence = false;
    point0ToPoint2Coincidence = false;
    point0ToPoint3Coincidence = false;
    point2ToPoint1Coincidence = false;
    if (std::fabs(inP0.x - inP2.x) < 0.0001 && std::fabs(inP0.y - inP2.y) < 0.0001) {
        outCoincidentPoints[0] = true;
        outCoincidentPoints[2] = true;
        point0ToPoint2Coincidence = true;
        pointToPointCoincidence = true;
    }
    if (std::fabs(inP0.x - inP3.x) < 0.0001 && std::fabs(inP0.y - inP3.y) < 0.0001) {
        outCoincidentPoints[0] = true;
        outCoincidentPoints[3] = true;
        point0ToPoint3Coincidence = true;
        pointToPointCoincidence = true;
    }
    if (std::fabs(inP1.x - inP2.x) < 0.0001 && std::fabs(inP1.y - inP2.y) < 0.0001) {
        outCoincidentPoints[1] = true;
        outCoincidentPoints[2] = true;
        point2ToPoint1Coincidence = true;
        pointToPointCoincidence = true;
    }
    if (std::fabs(inP1.x - inP3.x) < 0.0001 && std::fabs(inP1.y - inP3.y) < 0.0001) {
        outCoincidentPoints[1] = true;
        outCoincidentPoints[3] = true;
        pointToPointCoincidence = true;
    }
    vertical1 = false;
    vertical2 = false;
    temp1 = inP1.x - inP0.x;
    temp2 = inP3.x - inP2.x;
    if (temp1 > -0.00001 && temp1 < 0.00001) vertical1 = true;
    if (temp2 > -0.00001 && temp2 < 0.00001) vertical2 = true;
    if (vertical1 && vertical2) {
        if (std::fabs(inP0.x - inP2.x) < 0.0001) {
            if ((inP3.y - inP0.y) * (inP0.y - inP2.y) > 0) outCoincidentPoints[0] = true;
            if ((inP1.y - inP2.y) * (inP2.y - inP0.y) > 0) outCoincidentPoints[2] = true;
            if ((inP3.y - inP1.y) * (inP1.y - inP2.y) > 0) outCoincidentPoints[1] = true;
            if ((inP1.y - inP3.y) * (inP3.y - inP0.y) > 0) outCoincidentPoints[3] = true;
            if (pointToPointCoincidence) {
                if (point0ToPoint2Coincidence) return true;
                return (outCoincidentPoints[0] && !point0ToPoint3Coincidence) || (outCoincidentPoints[2] && !point2ToPoint1Coincidence);
            }
            return outCoincidentPoints[0] || outCoincidentPoints[2];
        }
        return point0ToPoint2Coincidence;
    }
    m1 = 0; b1 = 0; m2 = 0; b2 = 0;
    if (!vertical1) { m1 = (inP1.y - inP0.y) / (inP1.x - inP0.x); b1 = inP0.y - m1 * inP0.x; }
    if (!vertical2) { m2 = (inP3.y - inP2.y) / (inP3.x - inP2.x); b2 = inP2.y - m2 * inP2.x; }
    if (vertical1) {
        if ((inP1.y - inP3.y) * (inP3.y - inP0.y) > 0 && std::fabs(inP0.x - inP3.x) < 0.0001) outCoincidentPoints[3] = true;
        if ((inP1.y - inP2.y) * (inP2.y - inP0.y) > 0 && std::fabs(inP0.x - inP2.x) < 0.0001) {
            outCoincidentPoints[2] = true;
            if (pointToPointCoincidence) return point0ToPoint2Coincidence;
            return true;
        }
        outPIntersec.x = inP0.x;
        outPIntersec.y = m2 * outPIntersec.x + b2;
        if ((inP3.x - outPIntersec.x) * (outPIntersec.x - inP2.x) > 0) {
            if (std::fabs(inP1.y - outPIntersec.y) < 0.0001) outCoincidentPoints[1] = true;
            if (std::fabs(inP0.y - outPIntersec.y) < 0.0001) {
                outCoincidentPoints[0] = true;
                if (pointToPointCoincidence) return point0ToPoint2Coincidence;
                return true;
            }
            if ((inP1.y - outPIntersec.y) * (outPIntersec.y - inP0.y) > 0) {
                if (pointToPointCoincidence) return point0ToPoint2Coincidence;
                return true;
            }
        }
        return point0ToPoint2Coincidence;
    }
    if (vertical2) {
        if ((inP3.y - inP1.y) * (inP1.y - inP2.y) > 0 && std::fabs(inP2.x - inP1.x) < 0.0001) outCoincidentPoints[1] = true;
        if ((inP3.y - inP0.y) * (inP0.y - inP2.y) > 0 && std::fabs(inP2.x - inP0.x) < 0.0001) {
            outCoincidentPoints[0] = true;
            if (pointToPointCoincidence) return point0ToPoint2Coincidence;
            return true;
        }
        outPIntersec.x = inP2.x;
        outPIntersec.y = m1 * outPIntersec.x + b1;
        if ((inP1.x - outPIntersec.x) * (outPIntersec.x - inP0.x) > 0) {
            if (std::fabs(inP3.y - outPIntersec.y) < 0.0001) outCoincidentPoints[3] = true;
            if (std::fabs(inP2.y - outPIntersec.y) < 0.0001) {
                outCoincidentPoints[2] = true;
                if (pointToPointCoincidence) return point0ToPoint2Coincidence;
                return true;
            }
            if ((inP3.y - outPIntersec.y) * (outPIntersec.y - inP2.y) > 0) {
                if (pointToPointCoincidence) return point0ToPoint2Coincidence;
                return true;
            }
        }
        return point0ToPoint2Coincidence;
    }
    if (std::fabs(m2 - m1) < 0.00001) {
        if (std::fabs(b1 - b2) < 0.0001) {
            if ((inP3.x - inP0.x) * (inP0.x - inP2.x) >= 0) outCoincidentPoints[0] = true;
            if ((inP3.x - inP1.x) * (inP1.x - inP2.x) >= 0) outCoincidentPoints[1] = true;
            if ((inP1.x - inP2.x) * (inP2.x - inP0.x) >= 0) outCoincidentPoints[2] = true;
            if ((inP1.x - inP3.x) * (inP3.x - inP0.x) >= 0) outCoincidentPoints[3] = true;
            if (pointToPointCoincidence) {
                if (point0ToPoint2Coincidence) return true;
                return (outCoincidentPoints[0] && !point0ToPoint3Coincidence) || (outCoincidentPoints[2] && !point2ToPoint1Coincidence);
            }
            return outCoincidentPoints[0] || outCoincidentPoints[2];
        }
        return point0ToPoint2Coincidence;
    }

    intersec1 = false;
    intersec2 = false;
    if (m1 < 1) {
        outPIntersec.x = (b1 - b2) / (m2 - m1);
        outPIntersec.y = m1 * outPIntersec.x + b1;
        if ((inP1.x - outPIntersec.x) * (outPIntersec.x - inP0.x) >= 0) intersec1 = true;
    }
    else {
        outPIntersec.y = (b1 * m2 - b2 * m1) / (m2 - m1);
        outPIntersec.x = (outPIntersec.y - b1) / m1;
        if ((inP1.y - outPIntersec.y) * (outPIntersec.y - inP0.y) >= 0) intersec1 = true;
    }
    if (m2 < 1) {
        if ((inP3.x - outPIntersec.x) * (outPIntersec.x - inP2.x) >= 0) intersec2 = true;
    }
    else {
        if ((inP3.y - outPIntersec.y) * (outPIntersec.y - inP2.y) >= 0) intersec2 = true;
    }
    if (intersec1 && intersec2) {
        if (std::fabs(inP0.x - outPIntersec.x) < 0.0001 && std::fabs(inP0.y - outPIntersec.y) < 0.0001) {
            outCoincidentPoints[0] = true;
            if (pointToPointCoincidence) return point0ToPoint2Coincidence;
            return true;
        }
        if (std::fabs(inP1.x - outPIntersec.x) < 0.0001 && std::fabs(inP1.y - outPIntersec.y) < 0.0001) {
            outCoincidentPoints[1] = true;
            return point0ToPoint2Coincidence;
        }
        if (std::fabs(inP2.x - outPIntersec.x) < 0.0001 && std::fabs(inP2.y - outPIntersec.y) < 0.0001) {
            outCoincidentPoints[2] = true;
            if (pointToPointCoincidence) return point0ToPoint2Coincidence;
            return true;
        }
        if (std::fabs(inP3.x - outPIntersec.x) < 0.0001 && std::fabs(inP3.y - outPIntersec.y) < 0.0001) {
            outCoincidentPoints[3] = true;
            return point0ToPoint2Coincidence;
        }
        if (pointToPointCoincidence) return point0ToPoint2Coincidence;
        return true;
    }
    return point0ToPoint2Coincidence;
}

_Polygon2DWA* WeilerAthertonPolygonClipper::getClipPolyWA() { return clipPolyWA; }
_Polygon2DWA* WeilerAthertonPolygonClipper::getSubjectPolyWA() { return subjectPolyWA; }

void WeilerAthertonPolygonClipper::resetOutputPolygon(Polygon2D* polygon)
{
    for (long int i = 0; i < polygon->loops.size(); ++i) delete polygon->loops[i];
    polygon->loops.clear();
    polygon->nextLoop();
}

void WeilerAthertonPolygonClipper::appendNonEmptyContours(const Polygon2D* source, Polygon2D* target)
{
    bool hasOutputVertices = hasAnyVertex(target);
    for (long int i = 0; i < source->loops.size(); ++i) {
        _Polygon2DContour* contour = source->loops.get(i);
        if (contour->vertices.size() == 0) continue;
        if (hasOutputVertices) target->nextLoop();
        for (long int j = 0; j < contour->vertices.size(); ++j) {
            Vertex2D v = contour->vertices.get(j);
            target->addVertex(v.x, v.y, v.color.r(), v.color.g(), v.color.b());
        }
        hasOutputVertices = true;
    }
}

bool WeilerAthertonPolygonClipper::hasAnyVertex(const Polygon2D* polygon)
{
    for (long int i = 0; i < polygon->loops.size(); ++i) if (polygon->loops.get(i)->vertices.size() > 0) return true;
    return false;
}
