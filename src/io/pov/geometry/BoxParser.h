#ifndef __BOX_PARSER__
#define __BOX_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class BoxParser {
  public:
    static SimpleBody *parseBox();
    static SimpleBody *parseBox(ParserContext &ctx);
};

#endif
