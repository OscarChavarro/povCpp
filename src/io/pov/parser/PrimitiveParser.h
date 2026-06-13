#ifndef __PRIMITIVE_PARSER_H__
#define __PRIMITIVE_PARSER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "io/pov/context/ParserContext.h"

class PrimitiveParser {
  public:
    static double parseFloat();
    static double parseFloat(ParserContext &ctx);
    static void parseVector(Vector3Dd *givenVector);
    static void parseVector(Vector3Dd *givenVector, ParserContext &ctx);
    static void parseCoeffs(int order, double *givenCoeffs);
    static void parseCoeffs(int order, double *givenCoeffs, ParserContext &ctx);
    static void parseColor(ColorRgba *givenColor);
    static void parseColor(ColorRgba *givenColor, ParserContext &ctx);
};

#endif
