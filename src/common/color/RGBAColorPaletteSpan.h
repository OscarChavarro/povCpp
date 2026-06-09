#ifndef __RGBA_COLOR_PALETTE_SPAN_H__
#define __RGBA_COLOR_PALETTE_SPAN_H__

#include "common/color/RGBAColor.h"

class RGBAColorPaletteSpan {
  public:
    double start;
    double end;
    RGBAColor startColor;
    RGBAColor endColor;
};

#endif
