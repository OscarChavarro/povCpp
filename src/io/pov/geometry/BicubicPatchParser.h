#ifndef __BICUBIC_PATCH_PARSER__
#define __BICUBIC_PATCH_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class BicubicPatchParser {
  public:
    static SimpleBody *parseBicubicPatch();
    static SimpleBody *parseBicubicPatch(ParserContext &ctx);
};

#endif
