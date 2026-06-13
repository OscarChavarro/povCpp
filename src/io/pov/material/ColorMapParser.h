#ifndef __COLOR_MAP_PARSER_H__
#define __COLOR_MAP_PARSER_H__

#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "io/pov/context/ParserContext.h"

class ColorMapParser {
  public:
    static RGBAColorPalette *parseColorMap();
    static RGBAColorPalette *parseColorMap(ParserContext &ctx);
};

#endif
