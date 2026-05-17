#ifndef __COLOUR_H__
#define __COLOUR_H__


#include "common/RGBAColor.h"
#include "common/RGBAColorPalette.h"
#include "common/RGBAColorPaletteSpan.h"

class Color {
  public:
    template <typename ColorT, typename ScalarT>
    static inline void makeColor(ColorT *c, ScalarT r, ScalarT g, ScalarT b)
    {
        c->Red = r;
        c->Green = g;
        c->Blue = b;
        c->Alpha = 0;
    }

    static double colorDistance(RGBAColor *color1, RGBAColor *color2);
    static void addColor(RGBAColor *result, RGBAColor *color1, RGBAColor *color2);
    static void scaleColor(RGBAColor *result, RGBAColor *color, double factor);
    static void clipColor(RGBAColor *result, RGBAColor *color);
    static inline double fabsInline(double x);
};

#endif
