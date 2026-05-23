#ifndef __BICUBIC_PATCH_PARSER_H__
#define __BICUBIC_PATCH_PARSER_H__

class Geometry;
class ParserContext;

class BicubicPatchParser {
  public:
    static Geometry *parseBicubicPatch();
    static Geometry *parseBicubicPatch(ParserContext &ctx);
};

#endif
