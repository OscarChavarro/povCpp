#ifndef __TRIANGLE_PARSER_H__
#define __TRIANGLE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class TranslatedBody;

class TriangleParser {
  public:
    static TranslatedBody *parseTriangle();
    static TranslatedBody *parseTriangle(ParserContext &ctx);
};

#endif
