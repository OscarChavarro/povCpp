#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_Construct.h"
int _TriangulationTrapezoid::insidePolygon() {
    int rightBoundarySegmentIndex = this->rightSegmentIndex;

    if (status == ST_INVALID)
        return 0;
    if ((leftSegmentIndex <= 0) || (rightBoundarySegmentIndex <= 0))
        return 0;
    if (((upperLeftTrapezoidIndex <= 0) && (upperRightTrapezoidIndex <= 0)) || ((lowerLeftTrapezoidIndex <= 0) && (lowerRightTrapezoidIndex <= 0))) {
        return _Construct::greaterThan(
            &_Construct::segmentAt(rightBoundarySegmentIndex).endPoint,
            &_Construct::segmentAt(rightBoundarySegmentIndex).startPoint);
    }
    return 0;
}
