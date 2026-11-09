#ifndef __POLYHEDRAL_BOUNDED_SOLID_VERTEX__
#define __POLYHEDRAL_BOUNDED_SOLID_VERTEX__

#include "java/lang/String.h"

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class _PolyhedralBoundedSolidHalfEdge;

class _PolyhedralBoundedSolidVertex {
public:
    int id;
    Vector3Dd position;
    _PolyhedralBoundedSolidHalfEdge* emanatingHalfEdge;
    ColorRgb debugColor;

    _PolyhedralBoundedSolidVertex(const Vector3Dd& position, int id)
        : id(id), position(position), emanatingHalfEdge(0), debugColor(1, 0, 0) {}

    /**
    @return a description of the node, as Java `toString`
    */
    java::String toString() const;
};

#endif
