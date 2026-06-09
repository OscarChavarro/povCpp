#ifndef __RGBA_COLOR_PALETTE_SPAN_H__
#define __RGBA_COLOR_PALETTE_SPAN_H__

#include "common/color/ColorRgba.h"

class RGBAColorPaletteSpan {
  public:
    double start;
    double end;
    ColorRgba startColor;
    ColorRgba endColor;
};

#endif
