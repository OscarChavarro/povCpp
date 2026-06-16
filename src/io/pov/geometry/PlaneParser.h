#ifndef __PLANE_PARSER_H__
#define __PLANE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class TranslatedBody;

class PlaneParser {
  public:
    static TranslatedBody *parsePlane();
    static TranslatedBody *parsePlane(ParserContext &ctx);
};

#endif
