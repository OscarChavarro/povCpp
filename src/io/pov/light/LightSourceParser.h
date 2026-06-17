#ifndef __LIGHT_SOURCE_PARSER_H__
#define __LIGHT_SOURCE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class LightSourceParser {
  public:
    static SimpleBody *parseLightSource();
    static SimpleBody *parseLightSource(ParserContext &ctx);
};

#endif
