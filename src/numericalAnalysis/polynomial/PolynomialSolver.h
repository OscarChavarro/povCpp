#ifndef __VECT_H__
#define __VECT_H__

#include "numericalAnalysis/polynomial/Polynomial.h"
#include "numericalAnalysis/polynomial/PolynomialConstants.h"

class PolynomialSolver {
  public:
    static int solveQuadratic(const double *x, double *y);
    static int solveCubic(const double *x, double *y);
    static int solveQuartic(const double *x, double *y, double minValue = 0.0);
    static int solvePolynomial(
        int order, const double *coeffs, double *roots, double minValue);
    static inline double absoluteValue(double x);

  private:
    static int polynomialRemainder(
        const Polynomial *dividend, const Polynomial *divisor, Polynomial *remainder);
    static int hasDifficultCoefficients(int n, const double *coefficients);
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
};

#endif
