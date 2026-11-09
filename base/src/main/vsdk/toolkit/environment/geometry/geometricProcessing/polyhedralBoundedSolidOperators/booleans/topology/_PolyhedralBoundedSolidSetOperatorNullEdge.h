//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_OPERATOR_NULL_EDGE__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_OPERATOR_NULL_EDGE__

#include "java/lang/String.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"

class _PolyhedralBoundedSolidEdge;

/**
Class `_PolyhedralBoundedSolidSetOperatorNullEdge` decorates a null edge so it
can participate in the connect stage driven by the `sonea`/`soneb` sets of
program [MANT1988].15.1 and later consumed by section [MANT1988].15.7.

C++ counterpart of Java's `_PolyhedralBoundedSolidSetOperatorNullEdge`.
*/
class _PolyhedralBoundedSolidSetOperatorNullEdge {
private:
    static PolyhedralBoundedSolidNumericPolicy::ToleranceContext numericContext;

public:
    _PolyhedralBoundedSolidEdge* e;

    // Default constructible, as the elements of `java::ArrayList`
    _PolyhedralBoundedSolidSetOperatorNullEdge() : e(nullptr) {}
    explicit _PolyhedralBoundedSolidSetOperatorNullEdge(_PolyhedralBoundedSolidEdge* e);

    static void setNumericContext(
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext* context);

    /**
    Total order by the canonical end points and the midpoint of the edges,
    compared exactly (as Java `compareTo`).
    */
    int compareTo(const _PolyhedralBoundedSolidSetOperatorNullEdge& other) const;

    /**
    @return true if this edge goes before `other` (natural order of `ArrayList::sort`)
    */
    bool operator<(const _PolyhedralBoundedSolidSetOperatorNullEdge& other) const;

    java::String toString() const;
};

#endif
