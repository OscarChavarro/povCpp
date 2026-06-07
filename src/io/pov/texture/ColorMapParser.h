#ifndef __COLOR_MAP_PARSER_H__
#define __COLOR_MAP_PARSER_H__

class RGBAColorPalette;
class ParserContext;

class ColorMapParser {
  public:
    static RGBAColorPalette *parseColorMap();
    static RGBAColorPalette *parseColorMap(ParserContext &ctx);
};

#endif
