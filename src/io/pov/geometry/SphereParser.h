#ifndef __SPHERE_PARSER__
#define __SPHERE_PARSER__

#include "environment/geometry/Geometry.h"
#include "environment/scene/SimpleBody.h"
#include "io/pov/context/ParserContext.h"

class SphereParser {
  public:
    static SimpleBody *parseSphere();
    static SimpleBody *parseSphere(ParserContext &ctx);
};

#endif
