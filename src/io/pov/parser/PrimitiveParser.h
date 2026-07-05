#ifndef __PRIMITIVE_PARSER__
#define __PRIMITIVE_PARSER__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"

class PrimitiveParser {
  public:
    static double parseFloat();
    static double parseFloat(ParserContext &ctx);
    static void parseVector(Vector3Dd *givenVector);
    static void parseVector(Vector3Dd *givenVector, ParserContext &ctx);
    static void parseColor(ColorRgba *givenColor);
    static void parseColor(ColorRgba *givenColor, ParserContext &ctx);
};

#endif
