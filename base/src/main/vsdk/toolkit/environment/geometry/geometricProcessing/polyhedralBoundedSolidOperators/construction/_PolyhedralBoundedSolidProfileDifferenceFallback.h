#ifndef ___POLYHEDRAL_BOUNDED_SOLID_PROFILE_DIFFERENCE_FALLBACK__
#define ___POLYHEDRAL_BOUNDED_SOLID_PROFILE_DIFFERENCE_FALLBACK__

#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/construction/_PolyhedralBoundedSolidProfileDifferenceFallbackSpec.h"

class PolyhedralBoundedSolid;

/**
Structural-shape boolean fallback for extruded YZ profiles: detects the
profile-subtraction case where the general pipeline emits only degenerate
vertex/face rings and rebuilds the difference directly from the clipped
profile.

C++ counterpart of Java's `_PolyhedralBoundedSolidProfileDifferenceFallback`.
*/
class _PolyhedralBoundedSolidProfileDifferenceFallback
    : public _PolyhedralBoundedSolidOperator {
public:
    /**
    @return a new spec owned by the caller, or null when the case does not
    apply
    */
    static _PolyhedralBoundedSolidProfileDifferenceFallbackSpec*
    prepareProfileDifferenceFallbackSpec(
        PolyhedralBoundedSolid* minuend,
        PolyhedralBoundedSolid* subtrahend,
        int op);

    /**
    @param spec fallback spec, can be null
    @param result current boolean result
    @return `result`, or a new rebuilt solid that replaces it (the caller
    then owns both)
    */
    static PolyhedralBoundedSolid* applyProfileDifferenceFallbackIfNeeded(
        _PolyhedralBoundedSolidProfileDifferenceFallbackSpec* spec,
        PolyhedralBoundedSolid* result);

private:
    _PolyhedralBoundedSolidProfileDifferenceFallback();
    static bool isBetween(double value, double min, double max);
    static PolyhedralBoundedSolid* buildProfileDifferenceFallback(
        _PolyhedralBoundedSolidProfileDifferenceFallbackSpec* spec);
};

#endif
