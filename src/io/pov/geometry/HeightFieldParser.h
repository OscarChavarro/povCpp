#ifndef __HEIGHT_FIELD_PARSER__
#define __HEIGHT_FIELD_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/context/ParserContext.h"

class HeightFieldParser {
  public:
    static SimpleBodyBuilder *parseHeightField();
    static SimpleBodyBuilder *parseHeightField(ParserContext &ctx);
};

#endif
