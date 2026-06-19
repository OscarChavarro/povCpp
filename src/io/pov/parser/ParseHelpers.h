#ifndef __PARSE_HELPERS__
#define __PARSE_HELPERS__

#include "io/pov/context/ParserContext.h"

class ParseHelpers {
  public:
    static void getExpectedToken(int tokenId);
    static void getExpectedToken(int tokenId, ParserContext &ctx);
};

#endif
