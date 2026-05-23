#ifndef __POLY_PARSER_H__
#define __POLY_PARSER_H__

class Geometry;
class ParserContext;

class PolyParser {
  public:
    static Geometry *parsePoly(int order);
    static Geometry *parsePoly(int order, ParserContext &ctx);
};

#endif
