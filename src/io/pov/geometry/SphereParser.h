#ifndef __SPHERE_PARSER_H__
#define __SPHERE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SphereParser {
  public:
    static Geometry *parseSphere();
    static Geometry *parseSphere(ParserContext &ctx);
};

#endif
