#ifndef __VECT_H__
#define __VECT_H__

#include "common/Frame.h"

extern int solveQuadratic(DBL *x, DBL *y);
extern int solveCubic(DBL *x, DBL *y);
extern int solveQuartic(DBL *x, DBL *y);
extern int polysolve(int order, DBL *Coeffs, DBL *roots);

#endif
