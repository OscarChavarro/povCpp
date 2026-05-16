#ifndef __RGBA_COLOR_PALETTE_SPAN_H__
#define __RGBA_COLOR_PALETTE_SPAN_H__

#ifndef DBL
#define DBL double
#endif

#include "common/RGBAColor.h"

class RGBAColorPaletteSpan {
  public:
    DBL start, end;
    RGBAColor Start_Colour, End_Colour;
};

#endif
