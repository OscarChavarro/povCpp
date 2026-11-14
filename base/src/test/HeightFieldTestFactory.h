#ifndef VSDK_HEIGHT_FIELD_TEST_FACTORY_H
#define VSDK_HEIGHT_FIELD_TEST_FACTORY_H

#include "vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.h"

class HeightFieldTestFactory {
public:
    // 5 x 5 grid of the plane h = 1 + x/2 + z/4 in the box [1,3]x[0,5]x[1,3].
    static HeightFieldSolid createPlane();
    // 17 x 17 grid of rolling hills in the box [1,15]x[0,6]x[1,15].
    static HeightFieldSolid createHills();
};
#endif
