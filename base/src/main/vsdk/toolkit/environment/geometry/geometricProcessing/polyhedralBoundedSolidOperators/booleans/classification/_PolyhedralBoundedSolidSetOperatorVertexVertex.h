//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_OPERATOR_VERTEX_VERTEX__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_OPERATOR_VERTEX_VERTEX__

#include "java/lang/String.h"

class _PolyhedralBoundedSolidVertex;

/**
Stores one vertex/vertex coincidence from the `sonvv` set introduced by
program [MANT1988].15.1.

C++ counterpart of Java's `_PolyhedralBoundedSolidSetOperatorVertexVertex`
(a plain record here).
*/
class _PolyhedralBoundedSolidSetOperatorVertexVertex {
public:
    _PolyhedralBoundedSolidVertex* va;
    _PolyhedralBoundedSolidVertex* vb;

    _PolyhedralBoundedSolidSetOperatorVertexVertex() : va(nullptr), vb(nullptr) {}

    java::String toString() const;
};

#endif
