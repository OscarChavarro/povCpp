#ifndef __RAY_H__
#define __RAY_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Ray {
  public:
    Vector3Dd position;  // Xo  Yo  Zo
    Vector3Dd direction; // Xv  Yv  Zv
};

#endif
