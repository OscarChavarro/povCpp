#ifndef __TRIANGLE_PARSER_H__
#define __TRIANGLE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class TriangleParser {
  public:
    static SimpleBody *parseTriangle();
    static SimpleBody *parseTriangle(ParserContext &ctx);
};

#endif
