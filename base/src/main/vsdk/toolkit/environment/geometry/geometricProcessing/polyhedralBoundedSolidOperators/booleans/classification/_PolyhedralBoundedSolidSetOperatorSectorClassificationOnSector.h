//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_OPERATOR_SECTOR_CLASSIFICATION_ON_SECTOR__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_OPERATOR_SECTOR_CLASSIFICATION_ON_SECTOR__

#include "java/lang/String.h"

class _PolyhedralBoundedSolidHalfEdge;

/**
This class is used to store sector / sector neigborhood information for the
vertex/vertex classifier as proposed on section [MANT1988].15.5. and program
[MANT1988].15.6.

C++ counterpart of Java's
`_PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector`.
*/
class _PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector {
public:
    int secta;
    int sectb;
    int s1a;
    int s2a;
    int s1b;
    int s2b;
    bool intersect;
    _PolyhedralBoundedSolidHalfEdge* hea;
    _PolyhedralBoundedSolidHalfEdge* heb;
    bool wa;
    bool wb;
    static const int ON = 0;
    static const int OUT = 1;
    static const int IN = -1;

    _PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector();

    void fillCases();

    java::String toString() const;

private:
    static const char* label(int i);
};

#endif
