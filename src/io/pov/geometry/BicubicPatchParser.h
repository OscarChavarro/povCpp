#ifndef __BICUBIC_PATCH_PARSER_H__
#define __BICUBIC_PATCH_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class BicubicPatchParser {
  public:
    static Geometry *parseBicubicPatch();
    static Geometry *parseBicubicPatch(ParserContext &ctx);
};

#endif
