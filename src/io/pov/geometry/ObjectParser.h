#ifndef __OBJECT_PARSER__
#define __OBJECT_PARSER__

#include "environment/geometry/Geometry.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/volume/compound/BooleanSetOperations.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;
class Light;

class ObjectParser {
  public:
    static SimpleBody *parseShape();
    static SimpleBody *parseShape(ParserContext &ctx);
    static BoundedGeometry *parseObject();
    static BoundedGeometry *parseObject(ParserContext &ctx);
    static BoundedGeometry *parseComposite();
    static BoundedGeometry *parseComposite(ParserContext &ctx);
    static CSG *parseCsg(BooleanSetOperations type);
    static CSG *parseCsg(BooleanSetOperations type, ParserContext &ctx);
};

#endif
