#ifndef __SMOOTH_TRIANGLE_PARSER_H__
#define __SMOOTH_TRIANGLE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class SmoothTriangleParser {
  public:
    static SimpleBody *parseSmoothTriangle();
    static SimpleBody *parseSmoothTriangle(ParserContext &ctx);
};

#endif
