#include "ImplicitSolidTestFactory.h"

PolynomialSurface ImplicitSolidTestFactory::createTorus(bool useSturmSequences)
{
    PolynomialSurface result(4, useSturmSequences);
    double* a = result.getCoefficients();
    a[PolynomialSurface::getTermIndex(4, 4, 0, 0)] = 1;
    a[PolynomialSurface::getTermIndex(4, 0, 4, 0)] = 1;
    a[PolynomialSurface::getTermIndex(4, 0, 0, 4)] = 1;
    a[PolynomialSurface::getTermIndex(4, 2, 2, 0)] = 2;
    a[PolynomialSurface::getTermIndex(4, 2, 0, 2)] = 2;
    a[PolynomialSurface::getTermIndex(4, 0, 2, 2)] = 2;
    a[PolynomialSurface::getTermIndex(4, 2, 0, 0)] = -8.5;
    a[PolynomialSurface::getTermIndex(4, 0, 2, 0)] = -8.5;
    a[PolynomialSurface::getTermIndex(4, 0, 0, 2)] = 7.5;
    a[PolynomialSurface::getTermIndex(4, 0, 0, 0)] = 14.0625;
    return result;
}

double ImplicitSolidTestFactory::evaluateUnivariate(const double* coefficients, int order, double t)
{
    double result = 0;
    for (int i = 0; i <= order; ++i) result = result*t + coefficients[i];
    return result;
}
