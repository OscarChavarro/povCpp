#ifndef __HEIGHT_FIELD_PARSER_H__
#define __HEIGHT_FIELD_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class TranslatedBody;

class HeightFieldParser {
  public:
    static TranslatedBody *parseHeightField();
    static TranslatedBody *parseHeightField(ParserContext &ctx);
};

#endif
