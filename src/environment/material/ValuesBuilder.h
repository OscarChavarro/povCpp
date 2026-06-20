#ifndef __VALUES_BUILDER__
#define __VALUES_BUILDER__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class ValuesBuilder {
  public:
    static ColorRgba *getColor();
    static Vector3Dd *getVector();
    static double *getFloat();
};

#endif
