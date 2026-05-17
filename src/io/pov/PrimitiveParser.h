#ifndef __PRIMITIVE_PARSER_H__
#define __PRIMITIVE_PARSER_H__

#include "common/linealAlgebra/Vector3Dd.h"

struct RGBAColor;

class PrimitiveParser {
  public:
    static double parseFloat();
    static void parseVector(Vector3Dd *givenVector);
    static void parseCoeffs(int order, double *givenCoeffs);
    static void parseColour(RGBAColor *givenColour);
};

#endif
