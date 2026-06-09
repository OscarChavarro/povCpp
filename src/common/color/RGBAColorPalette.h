#ifndef __RGBA_COLOR_PALETTE_H__
#define __RGBA_COLOR_PALETTE_H__

#include "common/color/RGBAColorPaletteSpan.h"

class RGBAColorPalette {
  public:
    int numberOfEntries;
    RGBAColorPaletteSpan *colorMapEntries;
    bool transparencyFlag;
};

#endif
