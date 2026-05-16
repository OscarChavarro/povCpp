#ifndef __RGBA_COLOR_PALETTE_SPAN_H__
#define __RGBA_COLOR_PALETTE_SPAN_H__

using DBL = double;

#include "common/RGBAColor.h"

class RGBAColorPaletteSpan {
  public:
    DBL start, end;
    RGBAColor Start_Colour, End_Colour;
};

#endif
