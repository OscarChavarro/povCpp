#ifndef __VECT_H__
#define __VECT_H__

#include "common/Frame.h"

class PolynomialSolver {
  public:
    static int solveQuadratic(DBL *x, DBL *y);
    static int solveCubic(DBL *x, DBL *y);
    static int solveQuartic(DBL *x, DBL *y);
    static int polysolve(int order, DBL *coeffs, DBL *roots);

  private:
    static int modp(class polynomial *u, class polynomial *v, class polynomial *r);
    static int difficultCoeffs(int n, DBL *x);
    static int regulaFalsa(int order, DBL *coef, DBL a, DBL b, DBL *val);
    static void sbisect(int np, class polynomial *sseq, DBL minValue, DBL maxValue,
        int atmin, int atmax, DBL *roots);
    static int numchanges(int np, class polynomial *sseq, DBL a);
    static DBL polyeval(DBL x, int n, DBL *coeffs);
    static int buildsturm(int ord, class polynomial *sseq);
    static int visibleRoots(int np, class polynomial *sseq, int *atneg, int *atpos);
};

#endif
