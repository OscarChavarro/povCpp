#ifndef __COLOUR_H__
#define __COLOUR_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"

class ColorOperations {
  public:
    static double colorDistance(ColorRgba *color1, ColorRgba *color2);
    static void addColor(
        ColorRgba *result, ColorRgba *color1, ColorRgba *color2);
    static void scaleColor(ColorRgba *result, ColorRgba *color, double factor);
    static void clipColor(ColorRgba *result, ColorRgba *color);
    static inline double fabsInline(double x);
};

#endif
