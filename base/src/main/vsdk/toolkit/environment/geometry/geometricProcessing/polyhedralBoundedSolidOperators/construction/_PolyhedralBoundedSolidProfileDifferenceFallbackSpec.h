#ifndef ___POLYHEDRAL_BOUNDED_SOLID_PROFILE_DIFFERENCE_FALLBACK_SPEC__
#define ___POLYHEDRAL_BOUNDED_SOLID_PROFILE_DIFFERENCE_FALLBACK_SPEC__

#include "java/util/ArrayList.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

/**
Data needed to rebuild a profile-subtraction result: the minuend YZ profile
clipped at the cut, the cut position and the minuend bounds.

C++ counterpart of Java's `_PolyhedralBoundedSolidProfileDifferenceFallbackSpec`.
*/
class _PolyhedralBoundedSolidProfileDifferenceFallbackSpec {
public:
    java::ArrayList<Vector3Dd> clippedProfileAtCut;
    double xCut;
    double xMax;
    double minuendBounds[6];

    _PolyhedralBoundedSolidProfileDifferenceFallbackSpec(
        const java::ArrayList<Vector3Dd>& clippedProfileAtCut,
        double xCut,
        double xMax,
        const double* minuendBounds)
        : clippedProfileAtCut(clippedProfileAtCut), xCut(xCut), xMax(xMax)
    {
        for ( int i = 0; i < 6; i++ ) {
            this->minuendBounds[i] = minuendBounds[i];
        }
    }
};

#endif
