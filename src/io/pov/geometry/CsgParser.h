#ifndef __CSG_PARSER__
#define __CSG_PARSER__

#include "environment/geometry/volume/constructiveSolidGeometry/BooleanSetOperations.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"

class CsgParser {
  public:
    // isNested is true only for the recursive calls CsgParser::parse makes
    // on itself, when a union/intersection/difference token appears inside
    // another CSG block's own body; every other call site (an `object { ... }`
    // body, or a top-level `#declare`) leaves it at the default false. See
    // ConstructiveSolidGeometryByRaySegment::topLevel's doc comment for why
    // this distinction matters under -csgRoth.
    static SimpleBodyBuilder *parse(
        BooleanSetOperations booleanSetOperation,
        ParserContext &ctx,
        bool isNested = false);
};

#endif
