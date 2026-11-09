#ifndef __VERTEX_CHAIN__
#define __VERTEX_CHAIN__

#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
class _VertexChain {
  public:
    Vector2Dd point;
    int adjacentVertexIndices[4];
    int chainNodeIndicesByAdjacency[4];
    int adjacencySlotCount;
};

#endif
