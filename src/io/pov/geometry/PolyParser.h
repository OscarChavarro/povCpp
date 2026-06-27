#ifndef __POLY_PARSER__
#define __POLY_PARSER__

#include "environment/geometry/TransformedGeometry.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/context/ParserContext.h"

class PolyParser {
  public:
    static SimpleBodyBuilder *parsePoly(int order, ParserContext &ctx);

  private:
    static SimpleBodyBuilder *rebuildBodyWithGeometry(SimpleBodyBuilder *body, TransformedGeometry *geometry);
    static void parseCoeffs(int order, double *givenCoeffs, ParserContext &ctx);
};

#endif
