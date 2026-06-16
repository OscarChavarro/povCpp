#ifndef __BICUBIC_PATCH_PARSER_H__
#define __BICUBIC_PATCH_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class TranslatedBody;

class BicubicPatchParser {
  public:
    static TranslatedBody *parseBicubicPatch();
    static TranslatedBody *parseBicubicPatch(ParserContext &ctx);
};

#endif
