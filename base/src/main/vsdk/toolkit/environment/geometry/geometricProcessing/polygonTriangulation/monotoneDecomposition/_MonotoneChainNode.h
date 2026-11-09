#ifndef __MONOTONE_CHAIN_NODE__
#define __MONOTONE_CHAIN_NODE__

class _MonotoneChainNode {
  public:
    int vertexIndex;
    int nextNodeIndex;
    int previousNodeIndex;
    bool isMarked;
};

#endif
