#ifndef __VECT_H__
#define __VECT_H__

#include "common/Frame.h"

class PolynomialSolver {
  public:
    static int solveQuadratic(double *x, double *y);
    static int solveCubic(double *x, double *y);
    static int solveQuartic(double *x, double *y);
    static int polysolve(int order, double *coeffs, double *roots);
    static inline double absInline(double x);
    static inline double maxInline(double x, double y);

  private:
    static int modp(class polynomial *u, class polynomial *v, class polynomial *r);
    static int difficultCoeffs(int n, double *x);
    static int regulaFalsa(int order, double *coef, double a, double b, double *val);
    static void sbisect(int np, class polynomial *sseq, double minValue, double maxValue,
        int atmin, int atmax, double *roots);
    static int numchanges(int np, class polynomial *sseq, double a);
    static double polyeval(double x, int n, double *coeffs);
    static int buildsturm(int ord, class polynomial *sseq);
    static int visibleRoots(int np, class polynomial *sseq, int *atneg, int *atpos);
};

#endif
