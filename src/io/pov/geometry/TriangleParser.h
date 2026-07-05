#ifndef __TRIANGLE_PARSER__
#define __TRIANGLE_PARSER__

#include "io/pov/context/ParserContext.h"

class TriangleParser {
  public:
    static SimpleBodyBuilder *parseTriangle(ParserContext &ctx);
};

#endif
