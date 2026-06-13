#ifndef __QUADRIC_PARSER_H__
#define __QUADRIC_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class QuadricParser {
  public:
    static Geometry *parseQuadric();
    static Geometry *parseQuadric(ParserContext &ctx);
};

#endif
