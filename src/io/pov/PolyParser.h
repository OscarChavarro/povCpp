#ifndef __POLY_PARSER_H__
#define __POLY_PARSER_H__

class Geometry;

class PolyParser {
  public:
    static Geometry *parsePoly(int order);
};

#endif
