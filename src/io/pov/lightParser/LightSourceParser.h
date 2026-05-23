#ifndef __LIGHT_SOURCE_PARSER_H__
#define __LIGHT_SOURCE_PARSER_H__

class Geometry;
class ParserContext;

class LightSourceParser {
  public:
    static Geometry *parseLightSource();
    static Geometry *parseLightSource(ParserContext &ctx);
};

#endif
