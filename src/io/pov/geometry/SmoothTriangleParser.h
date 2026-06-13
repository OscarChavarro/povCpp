#ifndef __SMOOTH_TRIANGLE_PARSER_H__
#define __SMOOTH_TRIANGLE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SmoothTriangleParser {
  public:
    static Geometry *parseSmoothTriangle();
    static Geometry *parseSmoothTriangle(ParserContext &ctx);
};

#endif
