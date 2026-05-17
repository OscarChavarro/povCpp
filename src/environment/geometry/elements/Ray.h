#ifndef __RAY_H__
#define __RAY_H__

#include "common/linealAlgebra/Vector3Dd.h"

class Ray {
  public:
    Vector3Dd position;  /*  Xo  Yo  Zo  */
    Vector3Dd direction; /*  Xv  Yv  Zv  */
};

#endif
