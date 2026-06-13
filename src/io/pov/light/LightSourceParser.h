#ifndef __LIGHT_SOURCE_PARSER_H__
#define __LIGHT_SOURCE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class LightSourceParser {
  public:
    static Geometry *parseLightSource();
    static Geometry *parseLightSource(ParserContext &ctx);
};

#endif
