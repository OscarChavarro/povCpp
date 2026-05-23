#ifndef __POLYNOMIAL_H__
#define __POLYNOMIAL_H__

#include "common/LegacyBoolean.h"
#include "processing/PolynomialConstants.h"

class Polynomial {
  public:
    int ord;
    double coef[MAX_ORDER + 1];
};

#endif
