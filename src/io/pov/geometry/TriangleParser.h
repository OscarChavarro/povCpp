#ifndef __TRIANGLE_PARSER__
#define __TRIANGLE_PARSER__

#include "environment/geometry/Geometry.h"
#include "environment/scene/SimpleBody.h"
#include "io/pov/context/ParserContext.h"

class TriangleParser {
  public:
    static SimpleBody *parseTriangle();
    static SimpleBody *parseTriangle(ParserContext &ctx);
};

#endif
