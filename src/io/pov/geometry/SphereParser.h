#ifndef __SPHERE_PARSER_H__
#define __SPHERE_PARSER_H__

class Geometry;
class ParserContext;

class SphereParser {
  public:
    static Geometry *parseSphere();
    static Geometry *parseSphere(ParserContext &ctx);
};

#endif
