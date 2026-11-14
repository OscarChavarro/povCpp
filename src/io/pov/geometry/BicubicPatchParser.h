#ifndef __BICUBIC_PATCH_PARSER__
#define __BICUBIC_PATCH_PARSER__

#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/context/ParserContext.h"
#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.h"

class BicubicPatchParser {
  public:
    static SimpleBodyBuilder *parseBicubicPatch();
    static SimpleBodyBuilder *parseBicubicPatch(ParserContext &ctx);

  private:
    // The VITRAL subdivision of PovRay's bicubic_patch type 0 to 4.
    static BezierPatchSubdivision subdivisionOf(int patchType);
};

#endif
