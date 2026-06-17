#ifndef __HEIGHT_FIELD_PARSER_H__
#define __HEIGHT_FIELD_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class HeightFieldParser {
  public:
    static SimpleBody *parseHeightField();
    static SimpleBody *parseHeightField(ParserContext &ctx);
};

#endif
