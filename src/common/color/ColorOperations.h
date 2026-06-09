#ifndef __COLOUR_H__
#define __COLOUR_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "common/color/RGBAColorPalette.h"
#include "common/color/RGBAColorPaletteSpan.h"

class ColorOperations {
  public:
    static inline void makeColor(ColorRgba *c, double r, double g, double b) {
        c->setR(r);
        c->setG(g);
        c->setB(b);
        c->setA(0);
    }

    static double colorDistance(ColorRgba *color1, ColorRgba *color2);
    static void addColor(
        ColorRgba *result, ColorRgba *color1, ColorRgba *color2);
    static void scaleColor(ColorRgba *result, ColorRgba *color, double factor);
    static void clipColor(ColorRgba *result, ColorRgba *color);
    static inline double fabsInline(double x);
};

#endif
