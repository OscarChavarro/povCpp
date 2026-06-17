#ifndef __QUADRIC_PARSER_H__
#define __QUADRIC_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class QuadricParser {
  public:
    static SimpleBody *parseQuadric();
    static SimpleBody *parseQuadric(ParserContext &ctx);
};

#endif
