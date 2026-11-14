#ifndef VSDK_SUBDIVIDED_BEZIER_PATCH_NODE_H
#define VSDK_SUBDIVIDED_BEZIER_PATCH_NODE_H

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

// Node of the bounding sphere tree of an adaptively subdivided Bezier patch:
// a leaf holds the four corners of a flat piece, an interior node the indices
// of its two or four children in the tree's node list.
class SubdividedBezierPatchNode {
public:
    bool leaf;
    Vector3Dd center;
    double radiusSquared;
    Vector3Dd corners[4];
    long children[4];
    int childCount;
    SubdividedBezierPatchNode() : leaf(true), center(), radiusSquared(0), childCount(0)
    {
        for (int i = 0; i < 4; i++) children[i] = -1;
    }
};
#endif
