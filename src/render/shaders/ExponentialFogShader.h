#ifndef __EXPONENTIAL_FOG_SHADER__
#define __EXPONENTIAL_FOG_SHADER__

#include "vsdk/toolkit/common/color/ColorRgba.h"

class ExponentialFogShader {
public:
    static void shade(double distance, const ColorRgba *fogColor, double fogDistance,
        ColorRgba *color);
};

#endif
