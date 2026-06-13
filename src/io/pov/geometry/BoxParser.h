#ifndef __BOX_PARSER_H__
#define __BOX_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class BoxParser {
  public:
    static Geometry *parseBox();
    static Geometry *parseBox(ParserContext &ctx);
};

#endif
