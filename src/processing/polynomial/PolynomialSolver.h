#ifndef __VECT_H__
#define __VECT_H__

#include "processing/polynomial/PolynomialConstants.h"
#include "processing/polynomial/Polynomial.h"

class PolynomialSolver {
  public:
    static int solveQuadratic(double *x, double *y);
    static int solveCubic(double *x, double *y);
    static int solveQuartic(double *x, double *y, double minValue = 0.0);
    static int polysolve(
        int order, double *coeffs, double *roots, double minValue);
    static inline double absInline(double x);
    static inline double maxInline(double x, double y);

  private:
    static int modp(Polynomial *u, Polynomial *v, Polynomial *r);
    static int difficultCoeffs(int n, double *x);
    static int regulaFalsa(
        int order, double *coef, double a, double b, double *val);
    static void sbisect(int np, Polynomial *sseq, double minValue,
        double maxValue, int atmin, int atmax, double *roots);
    static int numchanges(int np, Polynomial *sseq, double a);
    static double polyeval(double x, int n, double *coeffs);
    static int buildsturm(int ord, Polynomial *sseq);
    static int visibleRoots(int np, Polynomial *sseq, int *atneg, int *atpos);
};

#endif
