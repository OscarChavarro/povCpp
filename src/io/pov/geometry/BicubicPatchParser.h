#ifndef __BICUBIC_PATCH_PARSER_H__
#define __BICUBIC_PATCH_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class BicubicPatchParser {
  public:
    static SimpleBody *parseBicubicPatch();
    static SimpleBody *parseBicubicPatch(ParserContext &ctx);
};

#endif
