#ifndef __VECT_H__
#define __VECT_H__

#include "common/Frame.h"

extern int solve_quadratic(DBL *x, DBL *y);
extern int solve_cubic(DBL *x, DBL *y);
extern int solve_quartic(DBL *x, DBL *y);
extern int polysolve(int order, DBL *Coeffs, DBL *roots);

#endif
