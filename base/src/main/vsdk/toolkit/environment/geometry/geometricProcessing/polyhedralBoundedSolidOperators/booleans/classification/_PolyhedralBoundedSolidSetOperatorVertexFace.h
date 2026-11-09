//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_OPERATOR_VERTEX_FACE__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_OPERATOR_VERTEX_FACE__

#include "java/lang/String.h"

class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidVertex;

/**
Stores one vertex/face coincidence from the `sonva`/`sonvb` sets introduced by
program [MANT1988].15.1.

C++ counterpart of Java's `_PolyhedralBoundedSolidSetOperatorVertexFace`
(a plain record here: Java derives it from `_PolyhedralBoundedSolidOperator`
without using it).
*/
class _PolyhedralBoundedSolidSetOperatorVertexFace {
public:
    _PolyhedralBoundedSolidVertex* v;
    _PolyhedralBoundedSolidFace* f;

    _PolyhedralBoundedSolidSetOperatorVertexFace() : v(nullptr), f(nullptr) {}

    java::String toString() const;
};

#endif
