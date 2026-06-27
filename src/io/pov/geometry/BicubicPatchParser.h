#ifndef __BICUBIC_PATCH_PARSER__
#define __BICUBIC_PATCH_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/context/ParserContext.h"

class BicubicPatchParser {
  public:
    static SimpleBodyBuilder *parseBicubicPatch();
    static SimpleBodyBuilder *parseBicubicPatch(ParserContext &ctx);
};

#endif
