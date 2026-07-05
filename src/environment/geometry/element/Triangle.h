#ifndef __TRIANGLE__
#define __TRIANGLE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Triangle {
  public:
    Triangle() :
        v0(0), v1(0), v2(0),
        normal(0.0, 1.0, 0.0),
        distance(0.0),
        dominantAxis(0),
        inverted(0),
        vpCached(0),
        vpNormDotOrigin(0.0),
        degenerateFlag(false)
    {}

    int v0;
    int v1;
    int v2;
    Vector3Dd normal;
    double distance;
    unsigned int dominantAxis : 2;
    unsigned int inverted : 1;
    unsigned int vpCached : 1;
    double vpNormDotOrigin;
    bool degenerateFlag;
};

#endif
