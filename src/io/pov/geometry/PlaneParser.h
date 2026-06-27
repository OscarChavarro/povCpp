#ifndef __PLANE_PARSER__
#define __PLANE_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/context/ParserContext.h"

class PlaneParser {
  public:
    static SimpleBodyBuilder *parsePlane();
    static SimpleBodyBuilder *parsePlane(ParserContext &ctx);
};

#endif
