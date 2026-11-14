#ifndef __POLY_PARSER__
#define __POLY_PARSER__

#include "io/pov/context/ParserContext.h"
#include "vsdk/toolkit/environment/geometry/volume/PolynomialSurface.h"

class PolyParser {
  public:
    static SimpleBodyBuilder *parsePoly(int order, ParserContext &ctx);

  private:
    static SimpleBodyBuilder *rebuildBodyWithGeometry(SimpleBodyBuilder *body, Geometry *geometry);
    // A copy of the polynomial that solves its rays with Sturm sequences.
    static PolynomialSurface *copyWithSturmSequences(const PolynomialSurface &surface);
    static void parseCoeffs(int order, double *givenCoeffs, ParserContext &ctx);
};

#endif
