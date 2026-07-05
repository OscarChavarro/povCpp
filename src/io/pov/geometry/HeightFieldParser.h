#ifndef __HEIGHT_FIELD_PARSER__
#define __HEIGHT_FIELD_PARSER__

#include "io/pov/context/ParserContext.h"

class HeightFieldParser {
  public:
    static SimpleBodyBuilder *parseHeightField();
    static SimpleBodyBuilder *parseHeightField(ParserContext &ctx);
};

#endif
