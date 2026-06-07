#ifndef __QUADRIC_PARSER_H__
#define __QUADRIC_PARSER_H__

class Geometry;
class ParserContext;

class QuadricParser {
  public:
    static Geometry *parseQuadric();
    static Geometry *parseQuadric(ParserContext &ctx);
};

#endif
