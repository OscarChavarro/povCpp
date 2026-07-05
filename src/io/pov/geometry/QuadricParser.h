#ifndef __QUADRIC_PARSER__
#define __QUADRIC_PARSER__

#include "io/pov/context/ParserContext.h"

class QuadricParser {
  public:
    static SimpleBodyBuilder *parseQuadric(ParserContext &ctx);
};

#endif
