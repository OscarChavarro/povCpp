#ifndef __PRIMITIVE_PARSER_H__
#define __PRIMITIVE_PARSER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class RGBAColor;
class ParserContext;

class PrimitiveParser {
  public:
    static double parseFloat();
    static double parseFloat(ParserContext &ctx);
    static void parseVector(Vector3Dd *givenVector);
    static void parseVector(Vector3Dd *givenVector, ParserContext &ctx);
    static void parseCoeffs(int order, double *givenCoeffs);
    static void parseCoeffs(int order, double *givenCoeffs, ParserContext &ctx);
    static void parseColor(RGBAColor *givenColor);
    static void parseColor(RGBAColor *givenColor, ParserContext &ctx);
};

#endif
