#ifndef __PLANE_PARSER__
#define __PLANE_PARSER__

#include "io/pov/context/ParserContext.h"

class PlaneParser {
  public:
    static SimpleBodyBuilder *parsePlane();
    static SimpleBodyBuilder *parsePlane(ParserContext &ctx);
};

#endif
