#ifndef __TRIANGULATION_SEGMENT__
#define __TRIANGULATION_SEGMENT__

#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_TriangulationTrapezoidQueryNode.h"
class _TriangulationSegment {
  public:
    Vector2Dd startPoint;
    Vector2Dd endPoint;
    bool hasBeenInserted;
    _TriangulationTrapezoidQueryNode *startPointQueryNode;
    _TriangulationTrapezoidQueryNode *endPointQueryNode;
    int nextSegmentIndex;
    int previousSegmentIndex;
};

#endif
