#ifndef __SPHERE_PARSER__
#define __SPHERE_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/context/ParserContext.h"

class SphereParser {
  public:
    static SimpleBodyBuilder *parseSphere();
    static SimpleBodyBuilder *parseSphere(ParserContext &ctx);
};

#endif
