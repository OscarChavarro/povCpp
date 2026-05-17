#ifndef __POLYNOMIAL_H__
#define __POLYNOMIAL_H__

#include "common/FrameConfig.h"

class Polynomial {
  public:
    int ord;
    double coef[MAX_ORDER + 1];
};

#endif
