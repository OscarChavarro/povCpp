#ifndef __POLYNOMIAL_H__
#define __POLYNOMIAL_H__

#include "numericalAnalysis/polynomial/PolynomialConstants.h"

class Polynomial {
  public:
    int order;
    double coefficients[PolynomialConstants::MAX_ORDER + 1];
};

#endif
