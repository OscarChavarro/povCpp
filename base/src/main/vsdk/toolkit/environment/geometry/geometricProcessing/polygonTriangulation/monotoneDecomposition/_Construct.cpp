#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_Construct.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_IncrementalSegmentInserter.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_InsertionBatchSchedule.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_RandomSegmentOrder.h"
java::ArrayList<_TriangulationTrapezoidQueryNode *> _Construct::queryNodes; /* Query structure */
java::ArrayList<_TriangulationTrapezoid *> _Construct::trapezoids;          /* Trapezoid structure */
java::ArrayList<_TriangulationSegment> _Construct::segments;             /* Segment table */

int _Construct::nextQueryNodeIndex;
int _Construct::nextTrapezoidIndex;

void _Construct::resetTrapezoidQueryNodes() {
    for (long i = 0; i < queryNodes.size(); i++) {
        delete queryNodes[i];
        queryNodes[i] = nullptr;
    }
    nextQueryNodeIndex = 1;
}

void _Construct::resetTrapezoids() {
    for (long i = 0; i < trapezoids.size(); i++) {
        delete trapezoids[i];
        trapezoids[i] = nullptr;
    }
    nextTrapezoidIndex = 1;
}

void _Construct::prepareStorage(int nseg) {
    // Keep 1-based indexing used by the original implementation.
    const int segCapacity = nseg + 1;
    const int trapCapacity = 16 * nseg + 16;
    const int queryCapacity = 32 * nseg + 32;

    if (segments.size() != segCapacity) {
        segments.clear();
        segments.reserve(segCapacity);
        for (int i = 0; i < segCapacity; ++i) {
            segments.add(_TriangulationSegment());
        }
    }

    if (trapezoids.size() != trapCapacity) {
        trapezoids.clear();
        trapezoids.reserve(trapCapacity);
        for (int i = 0; i < trapCapacity; ++i) {
            trapezoids.add(nullptr);
        }
    } else {
        for (long i = 0; i < trapezoids.size(); ++i) {
            trapezoids[i] = nullptr;
        }
    }

    if (queryNodes.size() != queryCapacity) {
        queryNodes.clear();
        queryNodes.reserve(queryCapacity);
        for (int i = 0; i < queryCapacity; ++i) {
            queryNodes.add(nullptr);
        }
    } else {
        for (long i = 0; i < queryNodes.size(); ++i) {
            queryNodes[i] = nullptr;
        }
    }
}

/* Return the maximum of the two points into the splitPoint structure */
int _Construct::max(Vector2Dd *yvalPoint, Vector2Dd *firstPoint,
                    Vector2Dd *secondPoint) {
    if (firstPoint->y > secondPoint->y + TRIANGULATOR_EPSILON)
        *yvalPoint = *firstPoint;
    else if (fpEqual(firstPoint->y, secondPoint->y)) {
        if (firstPoint->x > secondPoint->x + TRIANGULATOR_EPSILON)
            *yvalPoint = *firstPoint;
        else
            *yvalPoint = *secondPoint;
    } else
        *yvalPoint = *secondPoint;

    return 0;
}

/* Return the minimum of the two points into the splitPoint structure */
int _Construct::min(Vector2Dd *yvalPoint, Vector2Dd *firstPoint,
                    Vector2Dd *secondPoint) {
    if (firstPoint->y < secondPoint->y - TRIANGULATOR_EPSILON)
        *yvalPoint = *firstPoint;
    else if (fpEqual(firstPoint->y, secondPoint->y)) {
        if (firstPoint->x < secondPoint->x)
            *yvalPoint = *firstPoint;
        else
            *yvalPoint = *secondPoint;
    } else
        *yvalPoint = *secondPoint;

    return 0;
}

bool _Construct::greaterThan(Vector2Dd *firstPoint, Vector2Dd *secondPoint) {
    if (firstPoint->y > secondPoint->y + TRIANGULATOR_EPSILON)
        return true;
    else if (firstPoint->y < secondPoint->y - TRIANGULATOR_EPSILON)
        return false;
    else
        return (firstPoint->x > secondPoint->x);
}

bool _Construct::equalTo(Vector2Dd *firstPoint, Vector2Dd *secondPoint) {
    return (fpEqual(firstPoint->y, secondPoint->y) &&
            fpEqual(firstPoint->x, secondPoint->x));
}

bool _Construct::greaterThanEqualTo(Vector2Dd *firstPoint,
                                    Vector2Dd *secondPoint) {
    if (firstPoint->y > secondPoint->y + TRIANGULATOR_EPSILON)
        return true;
    else if (firstPoint->y < secondPoint->y - TRIANGULATOR_EPSILON)
        return false;
    else
        return (firstPoint->x >= secondPoint->x);
}

bool _Construct::lessThan(Vector2Dd *firstPoint, Vector2Dd *secondPoint) {
    if (firstPoint->y < secondPoint->y - TRIANGULATOR_EPSILON)
        return true;
    else if (firstPoint->y > secondPoint->y + TRIANGULATOR_EPSILON)
        return false;
    else
        return (firstPoint->x < secondPoint->x);
}

double _Construct::cross(const Vector2Dd &v0, const Vector2Dd &v1,
                         const Vector2Dd &v2) {
    return ((v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x));
}

double _Construct::dot(const Vector2Dd &v0, const Vector2Dd &v1) {
    return (v0.x * v1.x + v0.y * v1.y);
}

bool _Construct::fpEqual(double firstValue, double secondValue) {
    return (fabs(firstValue - secondValue) <= TRIANGULATOR_EPSILON);
}

_TriangulationSegment &_Construct::segmentAt(int index) { return segments[index]; }

_TriangulationTrapezoid *_Construct::trapezoidAt(int index) { return trapezoids[index]; }

void _Construct::setTrapezoidAt(int index, _TriangulationTrapezoid *trapezoid) {
    trapezoids[index] = trapezoid;
}

_TriangulationTrapezoidQueryNode *_Construct::allocateQueryNode() {
    if (nextQueryNodeIndex < queryNodes.size()) {
        queryNodes[nextQueryNodeIndex] = new _TriangulationTrapezoidQueryNode();
        return queryNodes[nextQueryNodeIndex++];
    }
    fprintf(stderr, "new TrapezoidQueryNode: Query-table overflow\n");
    return nullptr;
}

int _Construct::allocateTrapezoidIndex() {
    if (nextTrapezoidIndex >= trapezoids.size()) {
        return -1;
    }
    return nextTrapezoidIndex++;
}

/* Initialise the query structure (Q) and the trapezoid table (T)
 * when the first segment is added to start the trapezoidation. The
 * query-tree starts out with 4 trapezoids, one S-node and 2 Y-nodes
 *
 *                4
 *   -----------------------------------
 *                \
 *      1          \        2
 *                  \
 *   -----------------------------------
 *                3
 */
_TriangulationTrapezoidQueryNode *_Construct::initQueryStructure(int segmentIndex) {
    // [SEID1991].3 Bootstrap of T(S) and query DAG for the first insertion.
    _TriangulationTrapezoidQueryNode *i1;
    _TriangulationTrapezoidQueryNode *i2;
    _TriangulationTrapezoidQueryNode *i3;
    _TriangulationTrapezoidQueryNode *i4;
    _TriangulationTrapezoidQueryNode *i5;
    _TriangulationTrapezoidQueryNode *i6;
    _TriangulationTrapezoidQueryNode *i7;
    _TriangulationTrapezoidQueryNode *root;
    int t1;
    int t2;
    int t3;
    int t4;
    _TriangulationSegment *s = &segments[segmentIndex];

    nextQueryNodeIndex = 1;
    resetTrapezoids();
    resetTrapezoidQueryNodes();

    i1 = (nextQueryNodeIndex < QSIZE
              ? (queryNodes[nextQueryNodeIndex] = new _TriangulationTrapezoidQueryNode(), queryNodes[nextQueryNodeIndex++])
              : (fprintf(stderr,
                         "new TrapezoidQueryNode: Query-table overflow\n"),
                 (_TriangulationTrapezoidQueryNode *)nullptr));
    i1->queryNodeType = T_Y;
    max(&i1->splitPoint, &s->startPoint, &s->endPoint); /* root */
    root = i1;

    i1->rightChild = i2 =
        (nextQueryNodeIndex < QSIZE
             ? (queryNodes[nextQueryNodeIndex] = new _TriangulationTrapezoidQueryNode(), queryNodes[nextQueryNodeIndex++])
             : (fprintf(stderr,
                        "new TrapezoidQueryNode: Query-table overflow\n"),
                (_TriangulationTrapezoidQueryNode *)nullptr));
    i2->queryNodeType = T_SINK;
    i2->parent = i1;

    i1->leftChild = i3 =
        (nextQueryNodeIndex < QSIZE
             ? (queryNodes[nextQueryNodeIndex] = new _TriangulationTrapezoidQueryNode(), queryNodes[nextQueryNodeIndex++])
             : (fprintf(stderr,
                        "new TrapezoidQueryNode: Query-table overflow\n"),
                (_TriangulationTrapezoidQueryNode *)nullptr));
    i3->queryNodeType = T_Y;
    min(&i3->splitPoint, &s->startPoint, &s->endPoint); /* root */
    i3->parent = i1;

    i3->leftChild = i4 =
        (nextQueryNodeIndex < QSIZE
             ? (queryNodes[nextQueryNodeIndex] = new _TriangulationTrapezoidQueryNode(), queryNodes[nextQueryNodeIndex++])
             : (fprintf(stderr,
                        "new TrapezoidQueryNode: Query-table overflow\n"),
                (_TriangulationTrapezoidQueryNode *)nullptr));
    i4->queryNodeType = T_SINK;
    i4->parent = i3;

    i3->rightChild = i5 =
        (nextQueryNodeIndex < QSIZE
             ? (queryNodes[nextQueryNodeIndex] = new _TriangulationTrapezoidQueryNode(), queryNodes[nextQueryNodeIndex++])
             : (fprintf(stderr,
                        "new TrapezoidQueryNode: Query-table overflow\n"),
                (_TriangulationTrapezoidQueryNode *)nullptr));
    i5->queryNodeType = T_X;
    i5->segmentIndex = segmentIndex;
    i5->parent = i3;

    i5->leftChild = i6 =
        (nextQueryNodeIndex < QSIZE
             ? (queryNodes[nextQueryNodeIndex] = new _TriangulationTrapezoidQueryNode(), queryNodes[nextQueryNodeIndex++])
             : (fprintf(stderr,
                        "new TrapezoidQueryNode: Query-table overflow\n"),
                (_TriangulationTrapezoidQueryNode *)nullptr));
    i6->queryNodeType = T_SINK;
    i6->parent = i5;

    i5->rightChild = i7 =
        (nextQueryNodeIndex < QSIZE
             ? (queryNodes[nextQueryNodeIndex] = new _TriangulationTrapezoidQueryNode(), queryNodes[nextQueryNodeIndex++])
             : (fprintf(stderr,
                        "new TrapezoidQueryNode: Query-table overflow\n"),
                (_TriangulationTrapezoidQueryNode *)nullptr));
    i7->queryNodeType = T_SINK;
    i7->parent = i5;

    if (nextTrapezoidIndex >= trapezoids.size())
        return nullptr;
    t1 = nextTrapezoidIndex++; /* middle left */
    trapezoids[t1] = new _TriangulationTrapezoid();
    if (nextTrapezoidIndex >= trapezoids.size())
        return nullptr;
    t2 = nextTrapezoidIndex++; /* middle right */
    trapezoids[t2] = new _TriangulationTrapezoid();
    if (nextTrapezoidIndex >= trapezoids.size())
        return nullptr;
    t3 = nextTrapezoidIndex++; /* bottom-most */
    trapezoids[t3] = new _TriangulationTrapezoid();
    if (nextTrapezoidIndex >= trapezoids.size())
        return nullptr;
    t4 = nextTrapezoidIndex++; /* topmost */
    trapezoids[t4] = new _TriangulationTrapezoid();

    trapezoids[t1]->upperPoint = trapezoids[t2]->upperPoint = trapezoids[t4]->lowerPoint = i1->splitPoint;
    trapezoids[t1]->lowerPoint = trapezoids[t2]->lowerPoint = trapezoids[t3]->upperPoint = i3->splitPoint;
    trapezoids[t4]->upperPoint.y = (double)(TRIANGULATION_INFINITY);
    trapezoids[t4]->upperPoint.x = (double)(TRIANGULATION_INFINITY);
    trapezoids[t3]->lowerPoint.y = (double)-1 * (TRIANGULATION_INFINITY);
    trapezoids[t3]->lowerPoint.x = (double)-1 * (TRIANGULATION_INFINITY);
    trapezoids[t1]->rightSegmentIndex = trapezoids[t2]->leftSegmentIndex = segmentIndex;
    trapezoids[t1]->upperLeftTrapezoidIndex = trapezoids[t2]->upperLeftTrapezoidIndex = t4;
    trapezoids[t1]->lowerLeftTrapezoidIndex = trapezoids[t2]->lowerLeftTrapezoidIndex = t3;
    trapezoids[t4]->lowerLeftTrapezoidIndex = trapezoids[t3]->upperLeftTrapezoidIndex = t1;
    trapezoids[t4]->lowerRightTrapezoidIndex = trapezoids[t3]->upperRightTrapezoidIndex = t2;

    trapezoids[t1]->sinkNode = i6;
    trapezoids[t2]->sinkNode = i7;
    trapezoids[t3]->sinkNode = i4;
    trapezoids[t4]->sinkNode = i2;

    trapezoids[t1]->status = trapezoids[t2]->status = ST_VALID;
    trapezoids[t3]->status = trapezoids[t4]->status = ST_VALID;

    i2->trapezoidIndex = t4;
    i4->trapezoidIndex = t3;
    i6->trapezoidIndex = t1;
    i7->trapezoidIndex = t2;

    s->hasBeenInserted = true;
    return root;
}

int _Construct::findNewRoots(int segmentIndex) {
    // [SEID1991].3 Endpoint root maintenance in the query DAG.
    _TriangulationSegment *s = &segments[segmentIndex];
    int trapezoidIndex;

    if (s->hasBeenInserted)
        return 0;

    trapezoidIndex = s->startPointQueryNode->locateEndpoint(&s->startPoint, &s->endPoint);
    s->startPointQueryNode = trapezoids[trapezoidIndex]->sinkNode;

    trapezoidIndex = s->endPointQueryNode->locateEndpoint(&s->endPoint, &s->startPoint);
    s->endPointQueryNode = trapezoids[trapezoidIndex]->sinkNode;
    return 0;
}

/* Main routine to perform trapezoidation */
int _Construct::constructTrapezoids(int nseg) {
    // [SEID1991].3 Randomized incremental construction with staged insertion.
    int i;
    _TriangulationTrapezoidQueryNode *root;
    int h;

    prepareStorage(nseg);

    /* Add the first segment and get the query structure and trapezoid */
    /* list initialised */

    root = initQueryStructure(_RandomSegmentOrder::chooseSegment());

    for (i = 1; i <= nseg; i++)
        segments[i].startPointQueryNode = segments[i].endPointQueryNode = root;

    for (h = 1; h <= _InsertionBatchSchedule::mathLogStarN(nseg); h++) {
        for (i = _InsertionBatchSchedule::mathN(nseg, h - 1) + 1;
             i <= _InsertionBatchSchedule::mathN(nseg, h); i++)
            _IncrementalSegmentInserter::addSegment(
                    _RandomSegmentOrder::chooseSegment());

        /* Find a new root for each of the segment endpoints */
        for (i = 1; i <= nseg; i++)
            findNewRoots(i);
    }

    for (i = _InsertionBatchSchedule::mathN(
            nseg, _InsertionBatchSchedule::mathLogStarN(nseg)) +
             1;
         i <= nseg; i++)
        _IncrementalSegmentInserter::addSegment(
                _RandomSegmentOrder::chooseSegment());

    return 0;
}
