#ifndef __RGBA_COLOR_PALETTE_H__
#define __RGBA_COLOR_PALETTE_H__

#include "common/color/RGBAColorPaletteSpan.h"

class RGBAColorPalette {
  public:
    int numberOfEntries;
    RGBAColorPaletteSpan *Colour_Map_Entries;
    int transparencyFlag;
};

#endif
