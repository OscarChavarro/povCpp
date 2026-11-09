#ifndef __POLYHEDRAL_BOUNDED_SOLID_EDGE__
#define __POLYHEDRAL_BOUNDED_SOLID_EDGE__

#include "java/lang/String.h"

#include "vsdk/toolkit/common/color/ColorRgb.h"
class _PolyhedralBoundedSolidHalfEdge;

class _PolyhedralBoundedSolidEdge {
public:
    _PolyhedralBoundedSolidHalfEdge* rightHalf;
    _PolyhedralBoundedSolidHalfEdge* leftHalf;
    int id;
    ColorRgb debugColor;

    _PolyhedralBoundedSolidEdge();
    int getEndingVertexId() const;
    int getStartingVertexId() const;

    /**
    @return a description of the node, as Java `toString`
    */
    java::String toString() const;
};

#endif
