#ifndef __POLYNOMIAL__
#define __POLYNOMIAL__

#include "numericalAnalysis/polynomial/PolynomialConstants.h"

class Polynomial {
  public:
    int order;
    double coefficients[PolynomialConstants::MAX_ORDER + 1];
};

#endif
