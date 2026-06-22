#ifndef __HEIGHT_FIELD_PARSER__
#define __HEIGHT_FIELD_PARSER__

#include "environment/geometry/Geometry.h"
#include "environment/scene/SimpleBody.h"
#include "io/pov/context/ParserContext.h"

class HeightFieldParser {
  public:
    static SimpleBody *parseHeightField();
    static SimpleBody *parseHeightField(ParserContext &ctx);
};

#endif
