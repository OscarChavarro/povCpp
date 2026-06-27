#ifndef __POLY_PARSER__
#define __POLY_PARSER__

#include "environment/geometry/TransformedGeometry.h"
#include "environment/scene/SimpleBody.h"
#include "io/pov/context/ParserContext.h"

class PolyParser {
  public:
    static SimpleBody *parsePoly(int order, ParserContext &ctx);

  private:
    static SimpleBody *rebuildBodyWithGeometry(SimpleBody *body, TransformedGeometry *geometry);
    static void parseCoeffs(int order, double *givenCoeffs, ParserContext &ctx);
};

#endif
