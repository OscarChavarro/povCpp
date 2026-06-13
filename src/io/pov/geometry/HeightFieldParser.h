#ifndef __HEIGHT_FIELD_PARSER_H__
#define __HEIGHT_FIELD_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class HeightFieldParser {
  public:
    static Geometry *parseHeightField();
    static Geometry *parseHeightField(ParserContext &ctx);
};

#endif
