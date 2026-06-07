#ifndef __BOX_PARSER_H__
#define __BOX_PARSER_H__

class Geometry;
class ParserContext;

class BoxParser {
  public:
    static Geometry *parseBox();
    static Geometry *parseBox(ParserContext &ctx);
};

#endif
