#ifndef __PLANE_PARSER_H__
#define __PLANE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class PlaneParser {
  public:
    static Geometry *parsePlane();
    static Geometry *parsePlane(ParserContext &ctx);
};

#endif
