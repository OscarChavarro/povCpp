#ifndef __BOX_PARSER__
#define __BOX_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/context/ParserContext.h"

class BoxParser {
  public:
    static SimpleBodyBuilder *parseBox();
    static SimpleBodyBuilder *parseBox(ParserContext &ctx);
};

#endif
