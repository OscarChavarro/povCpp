// References: [SEID1991] Seidel, R. "A simple and Fast Randomized Algorithm
// for Computing Trapezoidal Decompositions and for Triangulating Polygons".

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_Construct.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_IncrementalSegmentInserter.h"
void _IncrementalSegmentInserter::normalizeSegmentForInsertion(_TriangulationSegment &segment,
                                                               bool &isSwapped) {
    isSwapped = false;
    if (_Construct::greaterThan(&segment.endPoint, &segment.startPoint)) {
        Vector2Dd swapPoint = segment.startPoint;
        _TriangulationTrapezoidQueryNode *tmp = segment.startPointQueryNode;
        segment.startPoint = segment.endPoint;
        segment.endPoint = swapPoint;
        segment.startPointQueryNode = segment.endPointQueryNode;
        segment.endPointQueryNode = tmp;
        isSwapped = true;
    }
}

void _IncrementalSegmentInserter::updateLowerNeighbourLinksAfterSplit(
    int splitTrapIndex, int originalUpperTrapIndex) {
    int tmpDownIndex;

    if (((tmpDownIndex = _Construct::trapezoids[splitTrapIndex]->lowerLeftTrapezoidIndex) > 0) &&
        (_Construct::trapezoids[tmpDownIndex]->upperLeftTrapezoidIndex == originalUpperTrapIndex))
        _Construct::trapezoids[tmpDownIndex]->upperLeftTrapezoidIndex = splitTrapIndex;
    if (((tmpDownIndex = _Construct::trapezoids[splitTrapIndex]->lowerLeftTrapezoidIndex) > 0) &&
        (_Construct::trapezoids[tmpDownIndex]->upperRightTrapezoidIndex == originalUpperTrapIndex))
        _Construct::trapezoids[tmpDownIndex]->upperRightTrapezoidIndex = splitTrapIndex;

    if (((tmpDownIndex = _Construct::trapezoids[splitTrapIndex]->lowerRightTrapezoidIndex) > 0) &&
        (_Construct::trapezoids[tmpDownIndex]->upperLeftTrapezoidIndex == originalUpperTrapIndex))
        _Construct::trapezoids[tmpDownIndex]->upperLeftTrapezoidIndex = splitTrapIndex;
    if (((tmpDownIndex = _Construct::trapezoids[splitTrapIndex]->lowerRightTrapezoidIndex) > 0) &&
        (_Construct::trapezoids[tmpDownIndex]->upperRightTrapezoidIndex == originalUpperTrapIndex))
        _Construct::trapezoids[tmpDownIndex]->upperRightTrapezoidIndex = splitTrapIndex;
}

int _IncrementalSegmentInserter::splitTrapezoidAtEndpoint(int segmentIndex,
                                                          _TriangulationSegment &segment,
                                                          bool useFirstEndpoint) {
    _TriangulationTrapezoidQueryNode *upperSinkNode;
    _TriangulationTrapezoidQueryNode *lowerSinkNode;
    _TriangulationTrapezoidQueryNode *querySplitNode;
    Vector2Dd *endpoint = useFirstEndpoint ? &segment.startPoint : &segment.endPoint;
    Vector2Dd *otherEndpoint = useFirstEndpoint ? &segment.endPoint : &segment.startPoint;

    int upperTrapezoid = (useFirstEndpoint ? segment.startPointQueryNode : segment.endPointQueryNode)
                             ->locateEndpoint(endpoint, otherEndpoint);
    int splitTrapIndex = _Construct::allocateTrapezoidIndex();
    if (splitTrapIndex < 0)
        return -1;

    _Construct::trapezoids[splitTrapIndex] = new _TriangulationTrapezoid();
    _Construct::trapezoids[splitTrapIndex]->status = ST_VALID;
    (*_Construct::trapezoids[splitTrapIndex]) = (*_Construct::trapezoids[upperTrapezoid]);

    _Construct::trapezoids[upperTrapezoid]->lowerPoint.y = _Construct::trapezoids[splitTrapIndex]->upperPoint.y =
        endpoint->y;
    _Construct::trapezoids[upperTrapezoid]->lowerPoint.x = _Construct::trapezoids[splitTrapIndex]->upperPoint.x =
        endpoint->x;

    _Construct::trapezoids[upperTrapezoid]->lowerLeftTrapezoidIndex = splitTrapIndex;
    _Construct::trapezoids[upperTrapezoid]->lowerRightTrapezoidIndex = 0;
    _Construct::trapezoids[splitTrapIndex]->upperLeftTrapezoidIndex = upperTrapezoid;
    _Construct::trapezoids[splitTrapIndex]->upperRightTrapezoidIndex = 0;

    updateLowerNeighbourLinksAfterSplit(splitTrapIndex, upperTrapezoid);

    upperSinkNode = _Construct::allocateQueryNode();
    lowerSinkNode = _Construct::allocateQueryNode();
    querySplitNode = _Construct::trapezoids[upperTrapezoid]->sinkNode;

    querySplitNode->queryNodeType = T_Y;
    querySplitNode->splitPoint = *endpoint;
    querySplitNode->segmentIndex = segmentIndex;
    querySplitNode->leftChild = lowerSinkNode;
    querySplitNode->rightChild = upperSinkNode;

    upperSinkNode->queryNodeType = T_SINK;
    upperSinkNode->trapezoidIndex = upperTrapezoid;
    upperSinkNode->parent = querySplitNode;

    lowerSinkNode->queryNodeType = T_SINK;
    lowerSinkNode->trapezoidIndex = splitTrapIndex;
    lowerSinkNode->parent = querySplitNode;

    _Construct::trapezoids[upperTrapezoid]->sinkNode = upperSinkNode;
    _Construct::trapezoids[splitTrapIndex]->sinkNode = lowerSinkNode;

    // Preserve original contract from pre-refactor code:
    // - first endpoint insertion returns the new lower trapezoid (tl)
    // - second endpoint insertion returns the upper trapezoid (tu)
    // This is required by addSegment traversal logic (tfirst/tlast semantics).
    return useFirstEndpoint ? splitTrapIndex : upperTrapezoid;
}

int _IncrementalSegmentInserter::locateOrInsertEndpointTrapezoid(
        int segmentIndex, _TriangulationSegment &segment, bool useFirstEndpoint,
        bool endpointAlreadyInserted, bool &wasEndpointInserted) {
    Vector2Dd *endpoint = useFirstEndpoint ? &segment.startPoint : &segment.endPoint;
    Vector2Dd *otherEndpoint = useFirstEndpoint ? &segment.endPoint : &segment.startPoint;

    wasEndpointInserted = false;
    if (endpointAlreadyInserted) {
        return (useFirstEndpoint ? segment.startPointQueryNode : segment.endPointQueryNode)
            ->locateEndpoint(endpoint, otherEndpoint);
    }

    wasEndpointInserted = true;
    return splitTrapezoidAtEndpoint(segmentIndex, segment, useFirstEndpoint);
}

bool
_IncrementalSegmentInserter::isLeftOf(int segmentIndex, Vector2Dd *queryPoint) {
    // [SEID1991].3 X-node decision primitive: point vs segment sided-ness.
    _TriangulationSegment *s = &_Construct::segments[segmentIndex];
    double area;

    if (_Construct::greaterThan(&s->endPoint, &s->startPoint)) /* segments. going upwards */
    {
        if (_Construct::fpEqual(s->endPoint.y, queryPoint->y)) {
            if (queryPoint->x < s->endPoint.x)
                area = 1.0;
            else
                area = -1.0;
        } else if (_Construct::fpEqual(s->startPoint.y, queryPoint->y)) {
            if (queryPoint->x < s->startPoint.x)
                area = 1.0;
            else
                area = -1.0;
        } else
            area = _Construct::cross(s->startPoint, s->endPoint, (*queryPoint));
    } else /* v0 > v1 */
    {
        if (_Construct::fpEqual(s->endPoint.y, queryPoint->y)) {
            if (queryPoint->x < s->endPoint.x)
                area = 1.0;
            else
                area = -1.0;
        } else if (_Construct::fpEqual(s->startPoint.y, queryPoint->y)) {
            if (queryPoint->x < s->startPoint.x)
                area = 1.0;
            else
                area = -1.0;
        } else
            area = _Construct::cross(s->endPoint, s->startPoint, (*queryPoint));
    }

    if (area > 0.0)
        return true;
    else
        return false;
}

/* Returns true if the corresponding endpoint of the given segment is */
/* already inserted into the segment tree. Use the simple test of */
/* whether the segment which shares this endpoint is already inserted */
bool _IncrementalSegmentInserter::inserted(int segmentIndex, int whichpt) {
    if (whichpt == SEGMENT_FIRST_ENDPOINT)
        return _Construct::segments[_Construct::segments[segmentIndex].previousSegmentIndex].hasBeenInserted;
    else
        return _Construct::segments[_Construct::segments[segmentIndex].nextSegmentIndex].hasBeenInserted;
}

/* Thread in the segment into the existing trapezoidation. The
 * limiting trapezoids are given by tfirst and tlast (which are the
 * trapezoids containing the two endpoints of the segment. Merges all
 * possible trapezoids which flank this segment and have been recently
 * divided because of its insertion
 */
int
_IncrementalSegmentInserter::mergeTrapezoids(int segmentIndex, int tfirst, int tlast,
                                             int side) {
    // [SEID1991].3 Merge contiguous trapezoids with compatible boundaries.
    int t;
    int tnext;
    bool cond;
    _TriangulationTrapezoidQueryNode *ptnext;

    /* First merge polys on the LHS */
    t = tfirst;
    while ((t > 0) && _Construct::greaterThanEqualTo(&_Construct::trapezoids[t]->lowerPoint, &_Construct::trapezoids[tlast]->lowerPoint)) {
        if (side == S_LEFT)
            cond = ((((tnext = _Construct::trapezoids[t]->lowerLeftTrapezoidIndex) > 0) &&
                     (_Construct::trapezoids[tnext]->rightSegmentIndex == segmentIndex)) ||
                    (((tnext = _Construct::trapezoids[t]->lowerRightTrapezoidIndex) > 0) &&
                     (_Construct::trapezoids[tnext]->rightSegmentIndex == segmentIndex)));
        else
            cond = ((((tnext = _Construct::trapezoids[t]->lowerLeftTrapezoidIndex) > 0) &&
                     (_Construct::trapezoids[tnext]->leftSegmentIndex == segmentIndex)) ||
                    (((tnext = _Construct::trapezoids[t]->lowerRightTrapezoidIndex) > 0) &&
                     (_Construct::trapezoids[tnext]->leftSegmentIndex == segmentIndex)));

        if (cond) {
            if ((_Construct::trapezoids[t]->leftSegmentIndex == _Construct::trapezoids[tnext]->leftSegmentIndex) &&
                (_Construct::trapezoids[t]->rightSegmentIndex == _Construct::trapezoids[tnext]->rightSegmentIndex)) /* good neighbours */
            {                                     /* merge them */
                /* Use the upper node as the new node i.e. t */

                ptnext = _Construct::trapezoids[tnext]->sinkNode->parent;

                if ( ptnext->leftChild == _Construct::trapezoids[tnext]->sinkNode)
                    ptnext->leftChild = _Construct::trapezoids[t]->sinkNode;
                else
                    ptnext->rightChild = _Construct::trapezoids[t]->sinkNode; /* redirect parent */

                /* Change the upper neighbours of the lower trapezoids */

                if ((_Construct::trapezoids[t]->lowerLeftTrapezoidIndex = _Construct::trapezoids[tnext]->lowerLeftTrapezoidIndex) > 0) {
                    if ( _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperLeftTrapezoidIndex == tnext)
                        _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperLeftTrapezoidIndex = t;
                    else if ( _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperRightTrapezoidIndex == tnext)
                            _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperRightTrapezoidIndex = t;
                }

                if ((_Construct::trapezoids[t]->lowerRightTrapezoidIndex = _Construct::trapezoids[tnext]->lowerRightTrapezoidIndex) > 0) {
                    if ( _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperLeftTrapezoidIndex == tnext)
                        _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperLeftTrapezoidIndex = t;
                    else if ( _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperRightTrapezoidIndex == tnext)
                            _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperRightTrapezoidIndex = t;
                }

                _Construct::trapezoids[t]->lowerPoint = _Construct::trapezoids[tnext]->lowerPoint;
                _Construct::trapezoids[tnext]->status = ST_INVALID; /* invalidate the lower */
                                               /* trapezium */
            } else                             /* not good neighbours */
                t = tnext;
        } else /* do not satisfy the outer if */
            t = tnext;

    } /* end-while */

    return 0;
}

/* Add in the new segment into the trapezoidation and update Q and T
 * structures. First locate the two endpoints of the segment in the
 * Q-structure. Then start from the topmost trapezoid and go down to
 * the  lower trapezoid dividing all the trapezoids in between .
 */
int
_IncrementalSegmentInserter::addSegment(int segmentIndex) {
    // [SEID1991].3 Incremental threading of one segment through T(S).
    _TriangulationSegment s;
    /*Segment *so = &Construct::segments[segmentIndex];*/
    _TriangulationTrapezoidQueryNode *sk;
    int tfirst;
    int tlast /*, tnext*/;
    int tfirstr = 0;
    int tlastr = 0;
    int tfirstl;
    int tlastl;
    _TriangulationTrapezoidQueryNode *i1;
    _TriangulationTrapezoidQueryNode *i2;
    int t;
    int tn;
    int tribot = 0;
    bool isSwapped = false;
    int tmptriseg;

    s = _Construct::segments[segmentIndex];
    normalizeSegmentForInsertion(s, isSwapped);

    bool insertedFirstEndpoint;
    const bool endpoint0AlreadyInserted =
        (isSwapped) ? inserted(segmentIndex, SEGMENT_LAST_ENDPOINT)
                    : inserted(segmentIndex, SEGMENT_FIRST_ENDPOINT);
    tfirst = locateOrInsertEndpointTrapezoid(segmentIndex, s, true,
                                             endpoint0AlreadyInserted,
                                             insertedFirstEndpoint);
    if (tfirst < 0)
        return -1;

    bool insertedLastEndpoint;
    const bool endpoint1AlreadyInserted =
        (isSwapped) ? inserted(segmentIndex, SEGMENT_FIRST_ENDPOINT)
                    : inserted(segmentIndex, SEGMENT_LAST_ENDPOINT);
    tlast = locateOrInsertEndpointTrapezoid(segmentIndex, s, false,
                                            endpoint1AlreadyInserted,
                                            insertedLastEndpoint);
    if (tlast < 0)
        return -1;
    tribot = insertedLastEndpoint ? 0 : 1;

    /* Thread the segment into the query tree creating a new X-node */
    /* First, split all the trapezoids which are intersected by s into */
    /* two */

    t = tfirst; /* topmost trapezoid */

    while ((t > 0) && _Construct::greaterThanEqualTo(&_Construct::trapezoids[t]->lowerPoint, &_Construct::trapezoids[tlast]->lowerPoint))
    /* traverse from top to bot */
    {
        int savedTrapezoidIndex;
        int savedNewTrapezoidIndex;
        sk = _Construct::trapezoids[t]->sinkNode;
        i1 = _Construct::allocateQueryNode(); /* left trapezoid sinkNode */
        i2 = _Construct::allocateQueryNode(); /* right trapezoid sinkNode */

        sk->queryNodeType = T_X;
        sk->segmentIndex = segmentIndex;
        sk->leftChild = i1;
        sk->rightChild = i2;

        i1->queryNodeType = T_SINK; /* left trapezoid (use existing one) */
        i1->trapezoidIndex = t;
        i1->parent = sk;

        i2->queryNodeType = T_SINK; /* right trapezoid (allocate new) */
        tn = _Construct::allocateTrapezoidIndex();
        if (tn < 0)
            return -1;
        _Construct::trapezoids[tn] = new _TriangulationTrapezoid();
        i2->trapezoidIndex = tn;
        _Construct::trapezoids[tn]->status = ST_VALID;
        i2->parent = sk;

        if (t == tfirst)
            tfirstr = tn;
        if (_Construct::equalTo(&_Construct::trapezoids[t]->lowerPoint, &_Construct::trapezoids[tlast]->lowerPoint))
            tlastr = tn;

        (*_Construct::trapezoids[tn]) = (*_Construct::trapezoids[t]);
        _Construct::trapezoids[t]->sinkNode = i1;
        _Construct::trapezoids[tn]->sinkNode = i2;
        savedTrapezoidIndex = t;
        savedNewTrapezoidIndex = tn;

        /* error */

        if ((_Construct::trapezoids[t]->lowerLeftTrapezoidIndex <= 0) && (_Construct::trapezoids[t]->lowerRightTrapezoidIndex <= 0)) /* case cannot arise */
        {
            fprintf(stderr, "addSegment: error\n");
            break;
        }

        /* only one trapezoid below. partition t into two and make the */
        /* two resulting trapezoids t and tn as the upper neighbours of */
        /* the sole lower trapezoid */

        else if ((_Construct::trapezoids[t]->lowerLeftTrapezoidIndex > 0) &&
                 (_Construct::trapezoids[t]->lowerRightTrapezoidIndex <= 0)) { /* Only one trapezoid below */
            if ((_Construct::trapezoids[t]->upperLeftTrapezoidIndex > 0) &&
                (_Construct::trapezoids[t]->upperRightTrapezoidIndex > 0)) {    /* continuation of a chain from abv. */
                if ( _Construct::trapezoids[t]->savedUpperNeighborIndex > 0) /* three upper neighbours */
                {
                    if ( _Construct::trapezoids[t]->savedUpperNeighborSide == S_LEFT) {
                        _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex;
                        _Construct::trapezoids[t]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[tn]->upperRightTrapezoidIndex = _Construct::trapezoids[t]->savedUpperNeighborIndex;

                        _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperRightTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                    } else /* intersects in the right */
                    {
                        _Construct::trapezoids[tn]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex;
                        _Construct::trapezoids[t]->upperRightTrapezoidIndex = _Construct::trapezoids[t]->upperLeftTrapezoidIndex;
                        _Construct::trapezoids[t]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->savedUpperNeighborIndex;

                        _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                        _Construct::trapezoids[_Construct::trapezoids[t]->upperRightTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                    }

                    _Construct::trapezoids[t]->savedUpperNeighborIndex = _Construct::trapezoids[tn]->savedUpperNeighborIndex = 0;
                } else /* No usave.... simple case */
                {
                    _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex;
                    _Construct::trapezoids[t]->upperRightTrapezoidIndex = _Construct::trapezoids[tn]->upperRightTrapezoidIndex = -1;
                    _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                }
            } else { /* fresh segments. or upward cusp */
                int tmpUpperIndex = _Construct::trapezoids[t]->upperLeftTrapezoidIndex;
                int td0;
                int td1;

                td0 = _Construct::trapezoids[tmpUpperIndex]->lowerLeftTrapezoidIndex;
                td1 = _Construct::trapezoids[tmpUpperIndex]->lowerRightTrapezoidIndex;
                if (td0 > 0 && td1 > 0) { /* upward cusp */
                    if ((_Construct::trapezoids[td0]->rightSegmentIndex > 0) &&
                        !isLeftOf(_Construct::trapezoids[td0]->rightSegmentIndex, &s.endPoint)) {
                        _Construct::trapezoids[t]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex = _Construct::trapezoids[tn]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerRightTrapezoidIndex = tn;
                    } else /* cusp going leftwards */
                    {
                        _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[tn]->upperRightTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                    }
                } else /* fresh segment */
                {
                    _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                    _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerRightTrapezoidIndex = tn;
                }
            }

            if ( _Construct::fpEqual(_Construct::trapezoids[t]->lowerPoint.y, _Construct::trapezoids[tlast]->lowerPoint.y) &&
                 _Construct::fpEqual(_Construct::trapezoids[t]->lowerPoint.x, _Construct::trapezoids[tlast]->lowerPoint.x) &&
                 tribot) { /* bottom forms a triangle */

                if (isSwapped)
                    tmptriseg = _Construct::segments[segmentIndex].previousSegmentIndex;
                else
                    tmptriseg = _Construct::segments[segmentIndex].nextSegmentIndex;

                if ((tmptriseg > 0) && isLeftOf(tmptriseg, &s.startPoint)) {
                    /* L-R downward cusp */
                    _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperLeftTrapezoidIndex = t;
                    _Construct::trapezoids[tn]->lowerLeftTrapezoidIndex = _Construct::trapezoids[tn]->lowerRightTrapezoidIndex = -1;
                } else {
                    /* R-L downward cusp */
                    _Construct::trapezoids[_Construct::trapezoids[tn]->lowerLeftTrapezoidIndex]->upperRightTrapezoidIndex = tn;
                    _Construct::trapezoids[t]->lowerLeftTrapezoidIndex = _Construct::trapezoids[t]->lowerRightTrapezoidIndex = -1;
                }
            } else {
                if ((_Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperLeftTrapezoidIndex > 0) && (_Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperRightTrapezoidIndex > 0)) {
                    if ( _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperLeftTrapezoidIndex == t) /* passes thru LHS */
                    {
                        _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->savedUpperNeighborIndex = _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperRightTrapezoidIndex;
                        _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->savedUpperNeighborSide = S_LEFT;
                    } else {
                        _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->savedUpperNeighborIndex = _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperLeftTrapezoidIndex;
                        _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->savedUpperNeighborSide = S_RIGHT;
                    }
                }
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperLeftTrapezoidIndex = t;
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperRightTrapezoidIndex = tn;
            }

            t = _Construct::trapezoids[t]->lowerLeftTrapezoidIndex;
        }

        else if ((_Construct::trapezoids[t]->lowerLeftTrapezoidIndex <= 0) &&
                 (_Construct::trapezoids[t]->lowerRightTrapezoidIndex > 0)) { /* Only one trapezoid below */
            if ((_Construct::trapezoids[t]->upperLeftTrapezoidIndex > 0) &&
                (_Construct::trapezoids[t]->upperRightTrapezoidIndex > 0)) {    /* continuation of a chain from abv. */
                if ( _Construct::trapezoids[t]->savedUpperNeighborIndex > 0) /* three upper neighbours */
                {
                    if ( _Construct::trapezoids[t]->savedUpperNeighborSide == S_LEFT) {
                        _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex;
                        _Construct::trapezoids[t]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[tn]->upperRightTrapezoidIndex = _Construct::trapezoids[t]->savedUpperNeighborIndex;

                        _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperRightTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                    } else /* intersects in the right */
                    {
                        _Construct::trapezoids[tn]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex;
                        _Construct::trapezoids[t]->upperRightTrapezoidIndex = _Construct::trapezoids[t]->upperLeftTrapezoidIndex;
                        _Construct::trapezoids[t]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->savedUpperNeighborIndex;

                        _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                        _Construct::trapezoids[_Construct::trapezoids[t]->upperRightTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                    }

                    _Construct::trapezoids[t]->savedUpperNeighborIndex = _Construct::trapezoids[tn]->savedUpperNeighborIndex = 0;
                } else /* No usave.... simple case */
                {
                    _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex;
                    _Construct::trapezoids[t]->upperRightTrapezoidIndex = _Construct::trapezoids[tn]->upperRightTrapezoidIndex = -1;
                    _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                }
            } else { /* fresh segments. or upward cusp */
                int tmpUpperIndex = _Construct::trapezoids[t]->upperLeftTrapezoidIndex;
                int td0;
                int td1;

                td0 = _Construct::trapezoids[tmpUpperIndex]->lowerLeftTrapezoidIndex;
                td1 = _Construct::trapezoids[tmpUpperIndex]->lowerRightTrapezoidIndex;
                if (td0 > 0 && td1 > 0) { /* upward cusp */
                    if ((_Construct::trapezoids[td0]->rightSegmentIndex > 0) &&
                        !isLeftOf(_Construct::trapezoids[td0]->rightSegmentIndex, &s.endPoint)) {
                        _Construct::trapezoids[t]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex = _Construct::trapezoids[tn]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerRightTrapezoidIndex = tn;
                    } else {
                        _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[tn]->upperRightTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                    }
                } else /* fresh segment */
                {
                    _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                    _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerRightTrapezoidIndex = tn;
                }
            }

            if ( _Construct::fpEqual(_Construct::trapezoids[t]->lowerPoint.y, _Construct::trapezoids[tlast]->lowerPoint.y) &&
                 _Construct::fpEqual(_Construct::trapezoids[t]->lowerPoint.x, _Construct::trapezoids[tlast]->lowerPoint.x) &&
                 tribot) { /* bottom forms a triangle */
                int tmpseg = 1;

                if (isSwapped)
                    tmptriseg = _Construct::segments[segmentIndex].previousSegmentIndex;
                else
                    tmptriseg = _Construct::segments[segmentIndex].nextSegmentIndex;

                if ((tmpseg > 0) && isLeftOf(tmpseg, &s.startPoint)) {
                    /* L-R downward cusp */
                    _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperLeftTrapezoidIndex = t;
                    _Construct::trapezoids[tn]->lowerLeftTrapezoidIndex = _Construct::trapezoids[tn]->lowerRightTrapezoidIndex = -1;
                } else {
                    /* R-L downward cusp */
                    _Construct::trapezoids[_Construct::trapezoids[tn]->lowerRightTrapezoidIndex]->upperRightTrapezoidIndex = tn;
                    _Construct::trapezoids[t]->lowerLeftTrapezoidIndex = _Construct::trapezoids[t]->lowerRightTrapezoidIndex = -1;
                }
            } else {
                if ((_Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperLeftTrapezoidIndex > 0) && (_Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperRightTrapezoidIndex > 0)) {
                    if ( _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperLeftTrapezoidIndex == t) /* passes thru LHS */
                    {
                        _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->savedUpperNeighborIndex = _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperRightTrapezoidIndex;
                        _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->savedUpperNeighborSide = S_LEFT;
                    } else {
                        _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->savedUpperNeighborIndex = _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperLeftTrapezoidIndex;
                        _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->savedUpperNeighborSide = S_RIGHT;
                    }
                }
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperLeftTrapezoidIndex = t;
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperRightTrapezoidIndex = tn;
            }

            t = _Construct::trapezoids[t]->lowerRightTrapezoidIndex;
        }

        /* two trapezoids below. Find out which one is intersected by */
        /* this segment and proceed down that one */

        else {
            /*int tmpseg = Construct::trapezoids[Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->rightSegmentIndex;*/
            double y0;
            double yt;
            Vector2Dd tmpPoint;
            int tnext;
            bool isD0;

            isD0 = false;
            if (_Construct::fpEqual(_Construct::trapezoids[t]->lowerPoint.y, s.startPoint.y)) {
                if ( _Construct::trapezoids[t]->lowerPoint.x > s.startPoint.x)
                    isD0 = true;
            } else {
                tmpPoint.y = y0 = _Construct::trapezoids[t]->lowerPoint.y;
                yt = (y0 - s.startPoint.y) / (s.endPoint.y - s.startPoint.y);
                tmpPoint.x = s.startPoint.x + yt * (s.endPoint.x - s.startPoint.x);

                if (_Construct::lessThan(&tmpPoint, &_Construct::trapezoids[t]->lowerPoint))
                    isD0 = true;
            }

            /* check continuity from the top so that the lower-neighbour */
            /* values are properly filled for the upper trapezoid */

            if ((_Construct::trapezoids[t]->upperLeftTrapezoidIndex > 0) &&
                (_Construct::trapezoids[t]->upperRightTrapezoidIndex > 0)) {    /* continuation of a chain from abv. */
                if ( _Construct::trapezoids[t]->savedUpperNeighborIndex > 0) /* three upper neighbours */
                {
                    if ( _Construct::trapezoids[t]->savedUpperNeighborSide == S_LEFT) {
                        _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex;
                        _Construct::trapezoids[t]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[tn]->upperRightTrapezoidIndex = _Construct::trapezoids[t]->savedUpperNeighborIndex;

                        _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperRightTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                    } else /* intersects in the right */
                    {
                        _Construct::trapezoids[tn]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex;
                        _Construct::trapezoids[t]->upperRightTrapezoidIndex = _Construct::trapezoids[t]->upperLeftTrapezoidIndex;
                        _Construct::trapezoids[t]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->savedUpperNeighborIndex;

                        _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                        _Construct::trapezoids[_Construct::trapezoids[t]->upperRightTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                    }

                    _Construct::trapezoids[t]->savedUpperNeighborIndex = _Construct::trapezoids[tn]->savedUpperNeighborIndex = 0;
                } else /* No usave.... simple case */
                {
                    _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex;
                    _Construct::trapezoids[tn]->upperRightTrapezoidIndex = -1;
                    _Construct::trapezoids[t]->upperRightTrapezoidIndex = -1;
                    _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = tn;
                }
            } else { /* fresh segments. or upward cusp */
                int tmpUpperIndex = _Construct::trapezoids[t]->upperLeftTrapezoidIndex;
                int td0;
                int td1;

                td0 = _Construct::trapezoids[tmpUpperIndex]->lowerLeftTrapezoidIndex;
                td1 = _Construct::trapezoids[tmpUpperIndex]->lowerRightTrapezoidIndex;
                if (td0 > 0 && td1 > 0) { /* upward cusp */
                    if ((_Construct::trapezoids[td0]->rightSegmentIndex > 0) &&
                        !isLeftOf(_Construct::trapezoids[td0]->rightSegmentIndex, &s.endPoint)) {
                        _Construct::trapezoids[t]->upperLeftTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex = _Construct::trapezoids[tn]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[_Construct::trapezoids[tn]->upperLeftTrapezoidIndex]->lowerRightTrapezoidIndex = tn;
                    } else {
                        _Construct::trapezoids[tn]->upperLeftTrapezoidIndex = _Construct::trapezoids[tn]->upperRightTrapezoidIndex = _Construct::trapezoids[t]->upperRightTrapezoidIndex = -1;
                        _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                    }
                } else /* fresh segment */
                {
                    _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerLeftTrapezoidIndex = t;
                    _Construct::trapezoids[_Construct::trapezoids[t]->upperLeftTrapezoidIndex]->lowerRightTrapezoidIndex = tn;
                }
            }

            if ( _Construct::fpEqual(_Construct::trapezoids[t]->lowerPoint.y, _Construct::trapezoids[tlast]->lowerPoint.y) &&
                 _Construct::fpEqual(_Construct::trapezoids[t]->lowerPoint.x, _Construct::trapezoids[tlast]->lowerPoint.x) && tribot) {
                /* this case arises only at the lowest trapezoid.. i.e.
                   tlast, if the lower endpoint of the segment is
                   already inserted in the structure */

                _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperLeftTrapezoidIndex = t;
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperRightTrapezoidIndex = -1;
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperLeftTrapezoidIndex = tn;
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperRightTrapezoidIndex = -1;

                _Construct::trapezoids[tn]->lowerLeftTrapezoidIndex = _Construct::trapezoids[t]->lowerRightTrapezoidIndex;
                _Construct::trapezoids[t]->lowerRightTrapezoidIndex = _Construct::trapezoids[tn]->lowerRightTrapezoidIndex = -1;

                tnext = _Construct::trapezoids[t]->lowerRightTrapezoidIndex;
            } else if (isD0)
            /* intersecting lowerLeftTrapezoidIndex */
            {
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperLeftTrapezoidIndex = t;
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperRightTrapezoidIndex = tn;
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperLeftTrapezoidIndex = tn;
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperRightTrapezoidIndex = -1;

                /* new code to determine the bottom neighbours of the */
                /* newly partitioned trapezoid */

                _Construct::trapezoids[t]->lowerRightTrapezoidIndex = -1;

                tnext = _Construct::trapezoids[t]->lowerLeftTrapezoidIndex;
            } else /* intersecting lowerRightTrapezoidIndex */
            {
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperLeftTrapezoidIndex = t;
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerLeftTrapezoidIndex]->upperRightTrapezoidIndex = -1;
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperLeftTrapezoidIndex = t;
                _Construct::trapezoids[_Construct::trapezoids[t]->lowerRightTrapezoidIndex]->upperRightTrapezoidIndex = tn;

                /* new code to determine the bottom neighbours of the */
                /* newly partitioned trapezoid */

                _Construct::trapezoids[tn]->lowerLeftTrapezoidIndex = _Construct::trapezoids[t]->lowerRightTrapezoidIndex;
                _Construct::trapezoids[tn]->lowerRightTrapezoidIndex = -1;

                tnext = _Construct::trapezoids[t]->lowerRightTrapezoidIndex;
            }

            t = tnext;
        }

        _Construct::trapezoids[savedTrapezoidIndex]->rightSegmentIndex = _Construct::trapezoids[savedNewTrapezoidIndex]->leftSegmentIndex =
            segmentIndex;
    } /* end-while */

    /* Now combine those trapezoids which share common segments. We can */
    /* use the pointers to the parent to connect these together. This */
    /* works only because all these new trapezoids have been formed */
    /* due to splitting by the segment, and hence have only one parent */

    tfirstl = tfirst;
    tlastl = tlast;
    mergeTrapezoids(segmentIndex, tfirstl, tlastl, S_LEFT);
    mergeTrapezoids(segmentIndex, tfirstr, tlastr, S_RIGHT);

    _Construct::segments[segmentIndex].hasBeenInserted = true;
    return 0;
}
