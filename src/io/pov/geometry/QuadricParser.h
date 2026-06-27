#ifndef __QUADRIC_PARSER__
#define __QUADRIC_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/context/ParserContext.h"

class QuadricParser {
  public:
    static SimpleBodyBuilder *parseQuadric(ParserContext &ctx);
};

#endif
