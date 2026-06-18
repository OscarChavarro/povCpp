#ifndef __PLANE_PARSER__
#define __PLANE_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class PlaneParser {
  public:
    static SimpleBody *parsePlane();
    static SimpleBody *parsePlane(ParserContext &ctx);
};

#endif
