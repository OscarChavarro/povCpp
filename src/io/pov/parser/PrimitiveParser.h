#ifndef __PRIMITIVE_PARSER_H__
#define __PRIMITIVE_PARSER_H__

#include "common/linealAlgebra/Vector3Dd.h"

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
    static void parseColour(RGBAColor *givenColour);
    static void parseColour(RGBAColor *givenColour, ParserContext &ctx);
};

#endif
