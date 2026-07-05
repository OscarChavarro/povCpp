#ifndef __SPHERE_PARSER__
#define __SPHERE_PARSER__

#include "io/pov/context/ParserContext.h"

class SphereParser {
  public:
    static SimpleBodyBuilder *parseSphere();
    static SimpleBodyBuilder *parseSphere(ParserContext &ctx);
};

#endif
