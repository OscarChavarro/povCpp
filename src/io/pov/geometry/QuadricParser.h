#ifndef __QUADRIC_PARSER_H__
#define __QUADRIC_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class TranslatedBody;

class QuadricParser {
  public:
    static TranslatedBody *parseQuadric();
    static TranslatedBody *parseQuadric(ParserContext &ctx);
};

#endif
