#ifndef __LIGHT_SOURCE_PARSER_H__
#define __LIGHT_SOURCE_PARSER_H__

#include "environment/light/Light.h"
#include "io/pov/context/ParserContext.h"

class LightSourceParser {
  public:
    static Light *parseLightSource();
    static Light *parseLightSource(ParserContext &ctx);
};

#endif
