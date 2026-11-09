#ifndef __TRIANGULATION_TRAPEZOID_QUERY_NODE__
#define __TRIANGULATION_TRAPEZOID_QUERY_NODE__

#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
class _TriangulationTrapezoidQueryNode {
  public:
    int queryNodeType;
    int segmentIndex;
    Vector2Dd splitPoint;
    int trapezoidIndex;
    _TriangulationTrapezoidQueryNode *parent;
    _TriangulationTrapezoidQueryNode *leftChild;
    _TriangulationTrapezoidQueryNode *rightChild;

    _TriangulationTrapezoidQueryNode()
        : queryNodeType(0), segmentIndex(0), splitPoint(), trapezoidIndex(0),
          parent(nullptr), leftChild(nullptr), rightChild(nullptr) {}
    int locateEndpoint(Vector2Dd *queryPoint, Vector2Dd *otherPoint);

  private:
    static bool isLeftOf(int segmentIndex, Vector2Dd *queryPoint);
};

#endif
