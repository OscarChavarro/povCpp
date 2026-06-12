#ifndef __VECT_H__
#define __VECT_H__

#include "numericalAnalysis/polynomial/Polynomial.h"
#include "numericalAnalysis/polynomial/PolynomialConstants.h"

class PolynomialSolver {
  public:
    static int solveQuadratic(const double *x, double *y);
    static int solveCubic(const double *x, double *y);
    static int solveQuartic(const double *x, double *y, double minValue = 0.0);
    static int polysolve(
        int order, const double *coeffs, double *roots, double minValue);
    static inline double absInline(double x);

  private:
    static int modp(const Polynomial *u, const Polynomial *v, Polynomial *r);
    static int difficultCoeffs(int n, const double *x);
    static int regulaFalsa(
        int order, const double *coef, double a, double b, double *val);
    static void sbisect(int np, const Polynomial *sseq, double minValue,
        double maxValue, int atmin, int atmax, double *roots);
    static int numchanges(int np, const Polynomial *sseq, double a);
    static double polyeval(double x, int n, const double *coeffs);
    static int buildsturm(int ord, Polynomial *sseq);
    static int visibleRoots(int np, const Polynomial *sseq, int *atneg, int *atpos);
};

#endif
