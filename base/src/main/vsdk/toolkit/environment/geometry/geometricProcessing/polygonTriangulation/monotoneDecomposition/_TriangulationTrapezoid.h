#ifndef __TRIANGULATION_TRAPEZOID__
#define __TRIANGULATION_TRAPEZOID__

#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
class _TriangulationTrapezoidQueryNode;

class _TriangulationTrapezoid {
  public:
    _TriangulationTrapezoid()
        : leftSegmentIndex(0), rightSegmentIndex(0), upperPoint(), lowerPoint(), upperLeftTrapezoidIndex(0), upperRightTrapezoidIndex(0), lowerLeftTrapezoidIndex(0), lowerRightTrapezoidIndex(0),
          sinkNode(nullptr), savedUpperNeighborIndex(0), savedUpperNeighborSide(0), status(1) {}
    int insidePolygon();
    int leftSegmentIndex;
    int rightSegmentIndex;
    Vector2Dd upperPoint;
    Vector2Dd lowerPoint;
    int upperLeftTrapezoidIndex;
    int upperRightTrapezoidIndex;
    int lowerLeftTrapezoidIndex;
    int lowerRightTrapezoidIndex;
    _TriangulationTrapezoidQueryNode *sinkNode;
    int savedUpperNeighborIndex;
    int savedUpperNeighborSide;
    int status;
};

#endif
