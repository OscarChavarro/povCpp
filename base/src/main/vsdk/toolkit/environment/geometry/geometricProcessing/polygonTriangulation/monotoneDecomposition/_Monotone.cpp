#include <cmath>
#include <cstring>

#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_Monotone.h"
_MonotoneChainNode _Monotone::monotoneChainNodes[TRAPEZOID_TABLE_SIZE]; /* Table to hold all the monotone */
/* polygons . Each monotone polygon */
/* is a circularly linked list */

_VertexChain _Monotone::vertexChains[SEGMENT_SIZE]; /* chain init. information. This */
                                     /* is used to decide which */
                                     /* monotone polygon to split if */
                                     /* there are several other */
                                     /* polygons touching at the same */
                                     /* vertex  */

int _Monotone::monotonePolygonEntryNode[SEGMENT_SIZE]; /* contains position of any vertex in */
                            /* the monotone chain for the polygon */
int _Monotone::visitedTrapezoids[TRAPEZOID_TABLE_SIZE];
int _Monotone::nextChainNodeIndex;
int _Monotone::nextOutputTriangleIndex;
int _Monotone::nextMonotonePolygonIndex;

int _Monotone::newMonotone(void)
/* return a new monotonePolygonEntryNode structure from the table */
{
    return ++nextMonotonePolygonIndex;
}

int _Monotone::newChainElement(void)
/* return a new chain element from the table */
{
    return ++nextChainNodeIndex;
}

double _Monotone::crossSine(const Vector2Dd &firstVector,
                            const Vector2Dd &secondVector) {
    return (firstVector.x * secondVector.y - secondVector.x * firstVector.y);
}

double _Monotone::length(const Vector2Dd &vector) {
    return sqrt(vector.x * vector.x + vector.y * vector.y);
}

double _Monotone::getAngle(Vector2Dd *basePoint, Vector2Dd *nextPoint,
                           Vector2Dd *otherPoint) {
    Vector2Dd edgeFromBasePoint;
    Vector2Dd edgeToOtherPoint;

    edgeFromBasePoint.x = nextPoint->x - basePoint->x;
    edgeFromBasePoint.y = nextPoint->y - basePoint->y;

    edgeToOtherPoint.x = otherPoint->x - basePoint->x;
    edgeToOtherPoint.y = otherPoint->y - basePoint->y;

    if (crossSine(edgeFromBasePoint, edgeToOtherPoint) >= 0) {
        /* sine is positive */
        return _Construct::dot(edgeFromBasePoint, edgeToOtherPoint) /
               length(edgeFromBasePoint) / length(edgeToOtherPoint);
    } else {
        return (-1.0 * _Construct::dot(edgeFromBasePoint, edgeToOtherPoint) /
                length(edgeFromBasePoint) / length(edgeToOtherPoint) -
                2);
    }
}

/* (v0, v1) is the new diagonal to be added to the polygon. Find which */
/* chain to use and return the positions of v0 and v1 in p and q */
int _Monotone::getVertexPositions(int v0, int v1, int *ip, int *iq) {
    _VertexChain *vp0, *vp1;
    int i;
    double angle;
    double temp;
    int tp = 0, tq = 0;

    vp0 = &vertexChains[v0];
    vp1 = &vertexChains[v1];

    /* p is identified as follows. Scan from (v0, v1) rightwards till */
    /* you hit the first segment starting from v0. That chain is the */
    /* chain of our interest */

    angle = -4.0;
    for (i = 0; i < 4; i++) {
        if (vp0->adjacentVertexIndices[i] <= 0)
            continue;
        if ((temp = getAngle(&vp0->point, &(vertexChains[vp0->adjacentVertexIndices[i]].point), &vp1->point)) >
            angle) {
            angle = temp;
            tp = i;
        }
    }

    *ip = tp;

    /* Do similar actions for q */
    angle = -4.0;
    for (i = 0; i < 4; i++) {
        if (vp1->adjacentVertexIndices[i] <= 0)
            continue;
        if ((temp = getAngle(&vp1->point, &(vertexChains[vp1->adjacentVertexIndices[i]].point), &vp0->point)) >
            angle) {
            angle = temp;
            tq = i;
        }
    }

    *iq = tq;
    return 0;
}

int _Monotone::makeNewMonotonePolygon(int mcur, int v0, int v1)
/* v0 and v1 are specified in anti-clockwise order with respect to
 * the current monotone polygon mcur. Split the current polygon into
 * two polygons using the diagonal (v0, v1)
 */
{
    int p;
    int q;
    int ip;
    int iq;
    int mnew = newMonotone();
    int i;
    int j;
    int nf0;
    int nf1;
    _VertexChain *vp0, *vp1;

    vp0 = &vertexChains[v0];
    vp1 = &vertexChains[v1];

    getVertexPositions(v0, v1, &ip, &iq);

    p = vp0->chainNodeIndicesByAdjacency[ip];
    q = vp1->chainNodeIndicesByAdjacency[iq];

    /* At this stage, we have got the positions of v0 and v1 in the */
    /* desired chain. Now modify the linked lists */

    i = newChainElement(); /* for the new list */
    j = newChainElement();

    monotoneChainNodes[i].vertexIndex = v0;
    monotoneChainNodes[j].vertexIndex = v1;

    monotoneChainNodes[i].nextNodeIndex = monotoneChainNodes[p].nextNodeIndex;
    monotoneChainNodes[monotoneChainNodes[p].nextNodeIndex].previousNodeIndex = i;
    monotoneChainNodes[i].previousNodeIndex = j;
    monotoneChainNodes[j].nextNodeIndex = i;
    monotoneChainNodes[j].previousNodeIndex = monotoneChainNodes[q].previousNodeIndex;
    monotoneChainNodes[monotoneChainNodes[q].previousNodeIndex].nextNodeIndex = j;

    monotoneChainNodes[p].nextNodeIndex = q;
    monotoneChainNodes[q].previousNodeIndex = p;

    nf0 = vp0->adjacencySlotCount;
    nf1 = vp1->adjacencySlotCount;

    vp0->adjacentVertexIndices[ip] = v1;

    vp0->chainNodeIndicesByAdjacency[nf0] = i;
    vp0->adjacentVertexIndices[nf0] = monotoneChainNodes[monotoneChainNodes[i].nextNodeIndex].vertexIndex;
    vp1->chainNodeIndicesByAdjacency[nf1] = j;
    vp1->adjacentVertexIndices[nf1] = v0;

    vp0->adjacencySlotCount++;
    vp1->adjacencySlotCount++;

    monotonePolygonEntryNode[mcur] = p;
    monotonePolygonEntryNode[mnew] = i;
    return mnew;
}

/* Main routine to get monotone polygons from the trapezoidation of
 * the polygon.
 */
int _Monotone::monotonateTrapezoids(int n) {
    // [SEID1991].2/.3 Build monotone subpolygons from trapezoid decomposition.
    int i;
    int trapezoidStart;

    memset((void *)vertexChains, 0, sizeof(vertexChains));
    memset((void *)visitedTrapezoids, 0, sizeof(visitedTrapezoids));
    memset((void *)monotoneChainNodes, 0, sizeof(monotoneChainNodes));
    memset((void *)monotonePolygonEntryNode, 0, sizeof(monotonePolygonEntryNode));

    /* First locate a trapezoid which lies inside the polygon */
    /* and which is triangular */
    for (i = 0; i < TRAPEZOID_TABLE_SIZE; i++) {
        if ( _Construct::trapezoidAt(i) && _Construct::trapezoidAt(i)->insidePolygon())
            break;
    }
    if (i == TRAPEZOID_TABLE_SIZE)
        return 0;
    trapezoidStart = i;

    /* Initialise the monotonePolygonEntryNode data-structure and start spanning all the */
    /* trapezoids within the polygon */

    for (i = 1; i <= n; i++) {
        monotoneChainNodes[i].previousNodeIndex = _Construct::segmentAt(i).previousSegmentIndex;
        monotoneChainNodes[i].nextNodeIndex = _Construct::segmentAt(i).nextSegmentIndex;
        monotoneChainNodes[i].vertexIndex = i;
        vertexChains[i].point = _Construct::segmentAt(i).startPoint;
        vertexChains[i].adjacentVertexIndices[0] = _Construct::segmentAt(i).nextSegmentIndex; /* nextNodeIndex vertex */
        vertexChains[i].chainNodeIndicesByAdjacency[0] = i;                       /* locn. of nextNodeIndex vertex */
        vertexChains[i].adjacencySlotCount = 1;
    }

    nextChainNodeIndex = n;
    nextMonotonePolygonIndex = 0;
    monotonePolygonEntryNode[0] = 1; /* position of any vertex in the first */
                /* chain  */

    /* traverse the polygon */
    if ( _Construct::trapezoidAt(trapezoidStart)->upperLeftTrapezoidIndex > 0) {
        traversePolygon(0, trapezoidStart, _Construct::trapezoidAt(trapezoidStart)->upperLeftTrapezoidIndex,
                        TR_FROM_UP);
    } else if ( _Construct::trapezoidAt(trapezoidStart)->lowerLeftTrapezoidIndex > 0) {
        traversePolygon(0, trapezoidStart, _Construct::trapezoidAt(trapezoidStart)->lowerLeftTrapezoidIndex,
                        TR_FROM_DN);
    }

    /* return the number of polygons created */
    return newMonotone();
}

/* recursively visit all the trapezoids */
int _Monotone::traversePolygon(int mcur, int trnum, int from, int dir) {
    // [SEID1991].3 Navigate adjacency in T(S) to extract monotone chains.
    _TriangulationTrapezoid *trapezoid;
    int mnew;
    int v0;
    int v1;
    int retval = 0;

    if ((trnum <= 0) || visitedTrapezoids[trnum])
        return 0;
    if (!_Construct::trapezoidAt(trnum))
        return 0;
    trapezoid = _Construct::trapezoidAt(trnum);

    visitedTrapezoids[trnum] = true;

    /* We have much more information available here. */
    /* rightSegmentIndex: goes upwards   */
    /* leftSegmentIndex: goes downwards */

    /* Initially assume that dir = TR_FROM_DN (from the leftChild) */
    /* Switch v0 and v1 if necessary afterwards */

    /* special cases for triangles with cusps at the opposite ends. */
    /* take care of this first */
    if ((trapezoid->upperLeftTrapezoidIndex <= 0) && (trapezoid->upperRightTrapezoidIndex <= 0)) {
        if ((trapezoid->lowerLeftTrapezoidIndex > 0) && (trapezoid->lowerRightTrapezoidIndex > 0)) {
            /* downward opening triangle */
            v0 = _Construct::trapezoidAt(trapezoid->lowerRightTrapezoidIndex)->leftSegmentIndex;
            v1 = trapezoid->leftSegmentIndex;
            if (from == trapezoid->lowerRightTrapezoidIndex) {
                mnew = makeNewMonotonePolygon(mcur, v1, v0);
                traversePolygon(mcur, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                traversePolygon(mnew, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
            } else {
                mnew = makeNewMonotonePolygon(mcur, v0, v1);
                traversePolygon(mcur, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                traversePolygon(mnew, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
            }
        } else {
            retval = SP_NOSPLIT; /* Just traverse all neighbours */
            traversePolygon(mcur, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
            traversePolygon(mcur, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
            traversePolygon(mcur, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
            traversePolygon(mcur, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
        }
    } else if ((trapezoid->lowerLeftTrapezoidIndex <= 0) && (trapezoid->lowerRightTrapezoidIndex <= 0)) {
        if ((trapezoid->upperLeftTrapezoidIndex > 0) && (trapezoid->upperRightTrapezoidIndex > 0)) {
            /* upward opening triangle */
            v0 = trapezoid->rightSegmentIndex;
            v1 = _Construct::trapezoidAt(trapezoid->upperLeftTrapezoidIndex)->rightSegmentIndex;
            if (from == trapezoid->upperRightTrapezoidIndex) {
                mnew = makeNewMonotonePolygon(mcur, v1, v0);
                traversePolygon(mcur, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                traversePolygon(mnew, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
            } else {
                mnew = makeNewMonotonePolygon(mcur, v0, v1);
                traversePolygon(mcur, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                traversePolygon(mnew, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
            }
        } else {
            retval = SP_NOSPLIT; /* Just traverse all neighbours */
            traversePolygon(mcur, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
            traversePolygon(mcur, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
            traversePolygon(mcur, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
            traversePolygon(mcur, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
        }
    } else if ((trapezoid->upperLeftTrapezoidIndex > 0) && (trapezoid->upperRightTrapezoidIndex > 0)) {
        if ((trapezoid->lowerLeftTrapezoidIndex > 0) && (trapezoid->lowerRightTrapezoidIndex > 0)) {
            /* downward + upward cusps */
            v0 = _Construct::trapezoidAt(trapezoid->lowerRightTrapezoidIndex)->leftSegmentIndex;
            v1 = _Construct::trapezoidAt(trapezoid->upperLeftTrapezoidIndex)->rightSegmentIndex;
            retval = SP_2UP_2DN;
            if (((dir == TR_FROM_DN) && (trapezoid->lowerRightTrapezoidIndex == from)) ||
                ((dir == TR_FROM_UP) && (trapezoid->upperRightTrapezoidIndex == from))) {
                mnew = makeNewMonotonePolygon(mcur, v1, v0);
                traversePolygon(mcur, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                traversePolygon(mcur, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                traversePolygon(mnew, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                traversePolygon(mnew, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
            } else {
                mnew = makeNewMonotonePolygon(mcur, v0, v1);
                traversePolygon(mcur, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                traversePolygon(mcur, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                traversePolygon(mnew, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                traversePolygon(mnew, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
            }
        } else { /* only downward cusp */
            if (_Construct::equalTo(&trapezoid->lowerPoint,
                                    &_Construct::segmentAt(trapezoid->leftSegmentIndex).endPoint)) {
                v0 = _Construct::trapezoidAt(trapezoid->upperLeftTrapezoidIndex)->rightSegmentIndex;
                v1 = _Construct::segmentAt(trapezoid->leftSegmentIndex).nextSegmentIndex;

                retval = SP_2UP_LEFT;
                if ((dir == TR_FROM_UP) && (trapezoid->upperLeftTrapezoidIndex == from)) {
                    mnew = makeNewMonotonePolygon(mcur, v1, v0);
                    traversePolygon(mcur, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mnew, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                } else {
                    mnew = makeNewMonotonePolygon(mcur, v0, v1);
                    traversePolygon(mcur, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mcur, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mcur, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mnew, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                }
            } else {
                v0 = trapezoid->rightSegmentIndex;
                v1 = _Construct::trapezoidAt(trapezoid->upperLeftTrapezoidIndex)->rightSegmentIndex;
                retval = SP_2UP_RIGHT;
                if ((dir == TR_FROM_UP) && (trapezoid->upperRightTrapezoidIndex == from)) {
                    mnew = makeNewMonotonePolygon(mcur, v1, v0);
                    traversePolygon(mcur, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mnew, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mnew, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                } else {
                    mnew = makeNewMonotonePolygon(mcur, v0, v1);
                    traversePolygon(mcur, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mcur, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mcur, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mnew, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                }
            }
        }
    } else if ((trapezoid->upperLeftTrapezoidIndex > 0) ||
               (trapezoid->upperRightTrapezoidIndex > 0)) /* no downward cusp */
    {
        if ((trapezoid->lowerLeftTrapezoidIndex > 0) && (trapezoid->lowerRightTrapezoidIndex > 0)) /* only upward cusp */
        {
            if (_Construct::equalTo(&trapezoid->upperPoint,
                                    &_Construct::segmentAt(trapezoid->leftSegmentIndex).startPoint)) {
                v0 = _Construct::trapezoidAt(trapezoid->lowerRightTrapezoidIndex)->leftSegmentIndex;
                v1 = trapezoid->leftSegmentIndex;
                retval = SP_2DN_LEFT;
                if (!((dir == TR_FROM_DN) && (trapezoid->lowerLeftTrapezoidIndex == from))) {
                    mnew = makeNewMonotonePolygon(mcur, v1, v0);
                    traversePolygon(mcur, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mcur, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mcur, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                } else {
                    mnew = makeNewMonotonePolygon(mcur, v0, v1);
                    traversePolygon(mcur, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mnew, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                }
            } else {
                v0 = _Construct::trapezoidAt(trapezoid->lowerRightTrapezoidIndex)->leftSegmentIndex;
                v1 = _Construct::segmentAt(trapezoid->rightSegmentIndex).nextSegmentIndex;

                retval = SP_2DN_RIGHT;
                if ((dir == TR_FROM_DN) && (trapezoid->lowerRightTrapezoidIndex == from)) {
                    mnew = makeNewMonotonePolygon(mcur, v1, v0);
                    traversePolygon(mcur, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mnew, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                } else {
                    mnew = makeNewMonotonePolygon(mcur, v0, v1);
                    traversePolygon(mcur, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mcur, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mcur, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                }
            }
        } else /* no cusp */
        {
            if ( _Construct::equalTo(&trapezoid->upperPoint,
                                     &_Construct::segmentAt(trapezoid->leftSegmentIndex).startPoint) &&
                 _Construct::equalTo(&trapezoid->lowerPoint,
                                     &_Construct::segmentAt(trapezoid->rightSegmentIndex).startPoint)) {
                v0 = trapezoid->rightSegmentIndex;
                v1 = trapezoid->leftSegmentIndex;
                retval = SP_SIMPLE_LRDN;
                if (dir == TR_FROM_UP) {
                    mnew = makeNewMonotonePolygon(mcur, v1, v0);
                    traversePolygon(mcur, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mcur, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mnew, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                } else {
                    mnew = makeNewMonotonePolygon(mcur, v0, v1);
                    traversePolygon(mcur, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mcur, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mnew, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                }
            } else if ( _Construct::equalTo(
                           &trapezoid->upperPoint,
                           &_Construct::segmentAt(trapezoid->rightSegmentIndex).endPoint) &&
                        _Construct::equalTo(
                           &trapezoid->lowerPoint,
                           &_Construct::segmentAt(trapezoid->leftSegmentIndex).endPoint)) {
                v0 = _Construct::segmentAt(trapezoid->rightSegmentIndex).nextSegmentIndex;
                v1 = _Construct::segmentAt(trapezoid->leftSegmentIndex).nextSegmentIndex;

                retval = SP_SIMPLE_LRUP;
                if (dir == TR_FROM_UP) {
                    mnew = makeNewMonotonePolygon(mcur, v1, v0);
                    traversePolygon(mcur, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mcur, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mnew, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                } else {
                    mnew = makeNewMonotonePolygon(mcur, v0, v1);
                    traversePolygon(mcur, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mcur, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                    traversePolygon(mnew, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                    traversePolygon(mnew, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                }
            } else /* no split possible */
            {
                retval = SP_NOSPLIT;
                traversePolygon(mcur, trapezoid->upperLeftTrapezoidIndex, trnum, TR_FROM_DN);
                traversePolygon(mcur, trapezoid->lowerLeftTrapezoidIndex, trnum, TR_FROM_UP);
                traversePolygon(mcur, trapezoid->upperRightTrapezoidIndex, trnum, TR_FROM_DN);
                traversePolygon(mcur, trapezoid->lowerRightTrapezoidIndex, trnum, TR_FROM_UP);
            }
        }
    }

    return retval;
}

/* For each monotone polygon, find the ymax and ymin (to determine the */
/* two y-monotone chains) and pass on this monotone polygon for greedy */
/* triangulation. */
/* Take care not to triangulate duplicate monotone polygons */
int _Monotone::triangulateMonotonePolygons(int numberOfVertices,
                                           int numberOfMonotonePolygons,
                                           int op[][3]) {
    // [SEID1991].2 Linear-time triangulation of monotone pieces.
    int i;
    Vector2Dd yMaxPoint;
    Vector2Dd yMinPoint;
    int p;
    int vfirst;
    int posmax;
    int v;
    int vcount;
    bool processed;

    nextOutputTriangleIndex = 0;
    for (i = 0; i < numberOfMonotonePolygons; i++) {
        vcount = 1;
        processed = false;
        vfirst = monotoneChainNodes[monotonePolygonEntryNode[i]].vertexIndex;
        yMaxPoint = yMinPoint = vertexChains[vfirst].point;
        posmax = monotonePolygonEntryNode[i];
        monotoneChainNodes[monotonePolygonEntryNode[i]].isMarked = true;
        p = monotoneChainNodes[monotonePolygonEntryNode[i]].nextNodeIndex;
        while ((v = monotoneChainNodes[p].vertexIndex) != vfirst) {
            if (monotoneChainNodes[p].isMarked) {
                processed = true;
                break; /* break from while */
            } else
                monotoneChainNodes[p].isMarked = true;

            if (_Construct::greaterThan(&vertexChains[v].point, &yMaxPoint)) {
                yMaxPoint = vertexChains[v].point;
                posmax = p;
            }
            if (_Construct::lessThan(&vertexChains[v].point, &yMinPoint)) {
                yMinPoint = vertexChains[v].point;
            }
            p = monotoneChainNodes[p].nextNodeIndex;
            vcount++;
        }

        if (processed) /* Go to nextNodeIndex polygon */
            continue;

        if (vcount == 3) /* already a triangle */
        {
            op[nextOutputTriangleIndex][0] = monotoneChainNodes[p].vertexIndex;
            op[nextOutputTriangleIndex][1] = monotoneChainNodes[monotoneChainNodes[p].nextNodeIndex].vertexIndex;
            op[nextOutputTriangleIndex][2] = monotoneChainNodes[monotoneChainNodes[p].previousNodeIndex].vertexIndex;
            nextOutputTriangleIndex++;
        } else /* triangulate the polygon */
        {
            v = monotoneChainNodes[monotoneChainNodes[posmax].nextNodeIndex].vertexIndex;
            if (_Construct::equalTo(&vertexChains[v].point,
                                    &yMinPoint)) { /* LHS is a single line */
                triangulateSinglePolygon(numberOfVertices, posmax, TRI_LHS, op);
            } else
                triangulateSinglePolygon(numberOfVertices, posmax, TRI_RHS, op);
        }
    }

    return nextOutputTriangleIndex;
}

/* A greedy corner-cutting algorithm to triangulate a y-monotone
 * polygon in O(n) time.
 * Joseph O-Rourke, Computational Geometry in C.
 */
int _Monotone::triangulateSinglePolygon(int nvert, int posmax, int side,
                                        int op[][3]) {
    // [SEID1991].2 Convex-corner clipping on one monotone chain.
    int v;
    int rc[SEGMENT_SIZE], ri = 0; /* reflex chain */
    int endv;
    int tmp;
    int chainNodeIndicesByAdjacency;

    if (side == TRI_RHS) /* RHS segment is a single segment */
    {
        rc[0] = monotoneChainNodes[posmax].vertexIndex;
        tmp = monotoneChainNodes[posmax].nextNodeIndex;
        rc[1] = monotoneChainNodes[tmp].vertexIndex;
        ri = 1;

        chainNodeIndicesByAdjacency = monotoneChainNodes[tmp].nextNodeIndex;
        v = monotoneChainNodes[chainNodeIndicesByAdjacency].vertexIndex;

        if ((endv = monotoneChainNodes[monotoneChainNodes[posmax].previousNodeIndex].vertexIndex) == 0)
            endv = nvert;
    } else /* LHS is a single segment */
    {
        tmp = monotoneChainNodes[posmax].nextNodeIndex;
        rc[0] = monotoneChainNodes[tmp].vertexIndex;
        tmp = monotoneChainNodes[tmp].nextNodeIndex;
        rc[1] = monotoneChainNodes[tmp].vertexIndex;
        ri = 1;

        chainNodeIndicesByAdjacency = monotoneChainNodes[tmp].nextNodeIndex;
        v = monotoneChainNodes[chainNodeIndicesByAdjacency].vertexIndex;

        endv = monotoneChainNodes[posmax].vertexIndex;
    }

    while ((v != endv) || (ri > 1)) {
        if (ri > 0) /* reflex chain is non-empty */
        {
            if ( _Construct::cross(vertexChains[v].point, vertexChains[rc[ri - 1]].point, vertexChains[rc[ri]].point) >
                 0) { /* convex corner: cut if off */
                op[nextOutputTriangleIndex][0] = rc[ri - 1];
                op[nextOutputTriangleIndex][1] = rc[ri];
                op[nextOutputTriangleIndex][2] = v;
                nextOutputTriangleIndex++;
                ri--;
            } else /* non-convex */
            {      /* add v to the chain */
                ri++;
                rc[ri] = v;
                chainNodeIndicesByAdjacency = monotoneChainNodes[chainNodeIndicesByAdjacency].nextNodeIndex;
                v = monotoneChainNodes[chainNodeIndicesByAdjacency].vertexIndex;
            }
        } else /* reflex-chain empty: add v to the */
        {      /* reflex chain and advance it  */
            rc[++ri] = v;
            chainNodeIndicesByAdjacency = monotoneChainNodes[chainNodeIndicesByAdjacency].nextNodeIndex;
            v = monotoneChainNodes[chainNodeIndicesByAdjacency].vertexIndex;
        }
    } /* end-while */

    /* reached the bottom vertex. Add in the triangle formed */
    op[nextOutputTriangleIndex][0] = rc[ri - 1];
    op[nextOutputTriangleIndex][1] = rc[ri];
    op[nextOutputTriangleIndex][2] = v;
    nextOutputTriangleIndex++;
    ri--;

    return 0;
}
