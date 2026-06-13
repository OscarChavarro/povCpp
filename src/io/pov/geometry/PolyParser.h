#ifndef __POLY_PARSER_H__
#define __POLY_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class PolyParser {
  public:
    static Geometry *parsePoly(int order);
    static Geometry *parsePoly(int order, ParserContext &ctx);
};

#endif
