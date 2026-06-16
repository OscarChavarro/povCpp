#ifndef __POLY_PARSER_H__
#define __POLY_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class TranslatedBody;

class PolyParser {
  public:
    static TranslatedBody *parsePoly(int order);
    static TranslatedBody *parsePoly(int order, ParserContext &ctx);
};

#endif
