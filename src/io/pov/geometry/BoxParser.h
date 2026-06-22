#ifndef __BOX_PARSER__
#define __BOX_PARSER__

#include "environment/geometry/Geometry.h"
#include "environment/scene/SimpleBody.h"
#include "io/pov/context/ParserContext.h"

class BoxParser {
  public:
    static SimpleBody *parseBox();
    static SimpleBody *parseBox(ParserContext &ctx);
};

#endif
