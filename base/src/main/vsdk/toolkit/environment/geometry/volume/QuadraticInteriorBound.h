#ifndef __QUADRATIC_INTERIOR_BOUND__
#define __QUADRATIC_INTERIOR_BOUND__

#include "vsdk/toolkit/common/numerics/FloatingInterval.h"

// Certified lower distance bound for the interior of an affine sphere.
// Inputs enclose the local squared length, gradient A^-T x, symmetric matrix
// A^-T A^-1, and an upper bound on its spectral norm. Returns zero if no
// positive bound can be verified. Caller checks the interval environment.
class QuadraticInteriorBound {
public:
    static double improve(double radius, const FloatingInterval& localSquared,
        const FloatingInterval gradient[3], const FloatingInterval quadratic[3][3],
        double spectralUpper);
};

#endif
