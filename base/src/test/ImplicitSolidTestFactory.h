#ifndef VSDK_IMPLICIT_SOLID_TEST_FACTORY_H
#define VSDK_IMPLICIT_SOLID_TEST_FACTORY_H

#include "vsdk/toolkit/environment/geometry/volume/PolynomialSurface.h"

// Reference implicit solids and helpers shared by the implicit solid tests.
class ImplicitSolidTestFactory {
public:
    // Torus of radii 2 and 0.5 around z: (s + 3.75)^2 - 16 (x^2 + y^2), s = |p|^2.
    static PolynomialSurface createTorus(bool useSturmSequences);
    // Horner evaluation of coefficients of t^order down to t^0.
    static double evaluateUnivariate(const double* coefficients, int order, double t);
};
#endif
