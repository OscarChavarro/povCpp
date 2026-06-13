#ifndef __POLYNOMIAL_SOLVER__
#define __POLYNOMIAL_SOLVER__

#include "numericalAnalysis/polynomial/Polynomial.h"
#include "numericalAnalysis/polynomial/PolynomialConstants.h"

class PolynomialSolver {
  private:
    static constexpr double EPSILON = 1.0e-10;
    static constexpr double COEFFICIENT_LIMIT = 1.0e-20;
    static constexpr int MAX_ITERATIONS = 50;
    static int polynomialRemainder(
        const Polynomial *dividend, const Polynomial *divisor, Polynomial *remainder);
    static int solveByRegulaFalsi(
        int order, const double *coefficients, double a, double b, double *root);
    static void bisectRoots(int sequenceLength, const Polynomial *sturmSequence,
        double minimumValue, double maximumValue, int changesAtMinimum,
        int changesAtMaximum, double *roots);
    static int countSignChanges(int sequenceLength, const Polynomial *sturmSequence,
        double value);
    static double evaluatePolynomial(double x, int order, const double *coeffs);
    static int buildSturmSequence(int order, Polynomial *sturmSequence);
    static int countVisibleRoots(
        int sequenceLength, const Polynomial *sturmSequence, int *atNegative, int *atPositive);

  public:
    static int solvePolynomial(
        int order, const double *coefficients, double *roots, double minValue);
};

#endif
