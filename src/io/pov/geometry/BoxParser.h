#ifndef __BOX_PARSER_H__
#define __BOX_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class TranslatedBody;

class BoxParser {
  public:
    static TranslatedBody *parseBox();
    static TranslatedBody *parseBox(ParserContext &ctx);
};

#endif
