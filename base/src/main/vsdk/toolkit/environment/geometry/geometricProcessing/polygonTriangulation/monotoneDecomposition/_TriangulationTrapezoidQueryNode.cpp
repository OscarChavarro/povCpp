#include <cstdio>

#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_Construct.h"
bool _TriangulationTrapezoidQueryNode::isLeftOf(int segmentIndex, Vector2Dd *queryPoint) {
    _TriangulationSegment *segment = &_Construct::segmentAt(segmentIndex);
    double area;

    if (_Construct::greaterThan(&segment->endPoint, &segment->startPoint)) {
        if (_Construct::fpEqual(segment->endPoint.y, queryPoint->y))
            area = (queryPoint->x < segment->endPoint.x) ? 1.0 : -1.0;
        else if (_Construct::fpEqual(segment->startPoint.y, queryPoint->y))
            area = (queryPoint->x < segment->startPoint.x) ? 1.0 : -1.0;
        else
            area = _Construct::cross(segment->startPoint, segment->endPoint, (*queryPoint));
    } else {
        if (_Construct::fpEqual(segment->endPoint.y, queryPoint->y))
            area = (queryPoint->x < segment->endPoint.x) ? 1.0 : -1.0;
        else if (_Construct::fpEqual(segment->startPoint.y, queryPoint->y))
            area = (queryPoint->x < segment->startPoint.x) ? 1.0 : -1.0;
        else
            area = _Construct::cross(segment->endPoint, segment->startPoint, (*queryPoint));
    }

    return area > 0.0;
}

int _TriangulationTrapezoidQueryNode::locateEndpoint(Vector2Dd *queryPoint,
                                                     Vector2Dd *otherPoint) {
    // [SEID1991].3 Query DAG point-location walk across X/Y nodes.
    switch (queryNodeType) {
    case T_SINK:
        return trapezoidIndex;

    case T_Y:
        if (_Construct::greaterThan(queryPoint, &splitPoint))
            return rightChild->locateEndpoint(queryPoint, otherPoint);
        else if (_Construct::equalTo(queryPoint, &splitPoint)) {
            if (_Construct::greaterThan(otherPoint, &splitPoint))
                return rightChild->locateEndpoint(queryPoint, otherPoint);
            else
                return leftChild->locateEndpoint(queryPoint, otherPoint);
        } else
            return leftChild->locateEndpoint(queryPoint, otherPoint);

    case T_X:
        if ( _Construct::equalTo(queryPoint, &_Construct::segmentAt(segmentIndex).startPoint) ||
             _Construct::equalTo(queryPoint, &_Construct::segmentAt(segmentIndex).endPoint)) {
            if (_Construct::fpEqual(queryPoint->y, otherPoint->y))
                return (otherPoint->x < queryPoint->x)
                           ? leftChild->locateEndpoint(queryPoint, otherPoint)
                           : rightChild->locateEndpoint(queryPoint, otherPoint);
            else if (isLeftOf(segmentIndex, otherPoint))
                return leftChild->locateEndpoint(queryPoint, otherPoint);
            else
                return rightChild->locateEndpoint(queryPoint, otherPoint);
        } else if (isLeftOf(segmentIndex, queryPoint))
            return leftChild->locateEndpoint(queryPoint, otherPoint);
        else
            return rightChild->locateEndpoint(queryPoint, otherPoint);

    default:
        fprintf(stderr, "Haggu !!!!!\n");
        break;
    }

    return 0;
}
