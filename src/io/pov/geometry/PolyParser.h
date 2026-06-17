#ifndef __POLY_PARSER_H__
#define __POLY_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class PolyParser {
  public:
    static SimpleBody *parsePoly(int order);
    static SimpleBody *parsePoly(int order, ParserContext &ctx);
};

#endif
