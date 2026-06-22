#ifndef __QUADRIC_PARSER__
#define __QUADRIC_PARSER__

#include "environment/geometry/Geometry.h"
#include "environment/scene/SimpleBody.h"
#include "io/pov/context/ParserContext.h"

class QuadricParser {
  public:
    static SimpleBody *parseQuadric();
    static SimpleBody *parseQuadric(ParserContext &ctx);
};

#endif
