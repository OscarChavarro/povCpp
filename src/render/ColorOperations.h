#ifndef __COLOR_OPERATIONS__
#define __COLOR_OPERATIONS__

#include "vsdk/toolkit/common/color/ColorRgba.h"

class ColorOperations {
  public:
    static double colorDistance(const ColorRgba *color1, const ColorRgba *color2);
    static void addColor(
        ColorRgba *result, const ColorRgba *color1, const ColorRgba *color2);
    static void scaleColor(ColorRgba *result, const ColorRgba *color, double factor);
    static void clipColor(ColorRgba *result, const ColorRgba *color);
    static inline double fabsInline(double x);
};

#endif
