#ifndef __PARSED_TRANSFORM_OP__
#define __PARSED_TRANSFORM_OP__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

// Records one translate/rotate/scale token parsed for an object body, in
// encounter order, so ObjectParser::parseObject can replay it against the
// released bodySteps/geometrySteps lists.
enum ParsedTransformKind {
    PARSED_TRANSLATE,
    PARSED_ROTATE,
    PARSED_SCALE
};

class ParsedTransformOp {
  public:
    ParsedTransformOp() : kind(PARSED_TRANSLATE), vector(0.0, 0.0, 0.0) {}
    ParsedTransformOp(ParsedTransformKind kind, const Vector3Dd &vector) :
        kind(kind), vector(vector) {}

    ParsedTransformKind getKind() const { return kind; }
    const Vector3Dd &getVector() const { return vector; }

  private:
    ParsedTransformKind kind;
    Vector3Dd vector;
};

#endif
