#ifndef __COLOR_MAP_PARSER__
#define __COLOR_MAP_PARSER__

#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/material/PovColorMap.h"

class ColorMapParser {
  private:
    static RGBAColorPalette *toRGBAColorPalette(PovColorMap *map);
  public:
    static RGBAColorPalette *parseColorMap(ParserContext &ctx);
};

#endif
