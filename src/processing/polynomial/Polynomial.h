#ifndef __POLYNOMIAL_H__
#define __POLYNOMIAL_H__

#include "processing/polynomial/PolynomialConstants.h"

class Polynomial {
  public:
    int ord;
    double coef[PolynomialConstants::MAX_ORDER + 1];
};

#endif
