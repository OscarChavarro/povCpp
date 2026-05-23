#ifndef __PLANE_PARSER_H__
#define __PLANE_PARSER_H__

class Geometry;
class ParserContext;

class PlaneParser {
  public:
    static Geometry *parsePlane();
    static Geometry *parsePlane(ParserContext &ctx);
};

#endif
