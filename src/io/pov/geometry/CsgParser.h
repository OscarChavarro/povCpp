#ifndef __CSG_PARSER__
#define __CSG_PARSER__

#include "environment/geometry/volume/compound/BooleanSetOperations.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "io/pov/context/ParserContext.h"

class CsgParser {
  public:
    static CSG *parse(BooleanSetOperations booleanSetOperation, ParserContext &ctx);
};

#endif
