#ifndef __DEFAULT_TEXTURE_PARSER__
#define __DEFAULT_TEXTURE_PARSER__

#include "io/pov/context/ParserContext.h"

class DefaultTextureParser {
  public:
    static void parseDefault();
    static void parseDefault(ParserContext &ctx);
};

#endif
