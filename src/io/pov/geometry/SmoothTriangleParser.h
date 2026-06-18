#ifndef __SMOOTH_TRIANGLE_PARSER__
#define __SMOOTH_TRIANGLE_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class SmoothTriangleParser {
  public:
    static SimpleBody *parseSmoothTriangle();
    static SimpleBody *parseSmoothTriangle(ParserContext &ctx);
};

#endif
