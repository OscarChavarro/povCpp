#ifndef __TRIANGLE_PARSER_H__
#define __TRIANGLE_PARSER_H__

class Geometry;
class ParserContext;

class TriangleParser {
  public:
    static Geometry *parseTriangle();
    static Geometry *parseTriangle(ParserContext &ctx);
};

#endif
