#ifndef __LIGHT_SOURCE_PARSER_H__
#define __LIGHT_SOURCE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class TranslatedBody;

class LightSourceParser {
  public:
    static TranslatedBody *parseLightSource();
    static TranslatedBody *parseLightSource(ParserContext &ctx);
};

#endif
