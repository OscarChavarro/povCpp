#ifndef __SPHERE_PARSER_H__
#define __SPHERE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class SphereParser {
  public:
    static SimpleBody *parseSphere();
    static SimpleBody *parseSphere(ParserContext &ctx);
};

#endif
