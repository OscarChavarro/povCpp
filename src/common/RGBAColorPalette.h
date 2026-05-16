#ifndef __RGBA_COLOR_PALETTE_H__
#define __RGBA_COLOR_PALETTE_H__

#include "common/RGBAColorPaletteSpan.h"

class RGBAColorPalette {
  public:
    int Number_Of_Entries;
    RGBAColorPaletteSpan *Colour_Map_Entries;
    int Transparency_Flag;
};

#endif
