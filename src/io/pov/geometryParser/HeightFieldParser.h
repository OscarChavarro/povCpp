#ifndef __HEIGHT_FIELD_PARSER_H__
#define __HEIGHT_FIELD_PARSER_H__

class Geometry;
class ParserContext;

class HeightFieldParser {
  public:
    static Geometry *parseHeightField();
    static Geometry *parseHeightField(ParserContext &ctx);
};

#endif
