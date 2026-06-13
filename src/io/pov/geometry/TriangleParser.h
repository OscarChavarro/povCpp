#ifndef __TRIANGLE_PARSER_H__
#define __TRIANGLE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class TriangleParser {
  public:
    static Geometry *parseTriangle();
    static Geometry *parseTriangle(ParserContext &ctx);
};

#endif
