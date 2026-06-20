#ifndef __OBJECT_PARSER__
#define __OBJECT_PARSER__

#include "environment/geometry/BoundedGeometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class ObjectParser {
  public:
    static SimpleBody *parseShape();
    static SimpleBody *parseShape(ParserContext &ctx);
    static BoundedGeometry *parseObject();
    static BoundedGeometry *parseObject(ParserContext &ctx);
    static BoundedGeometry *parseComposite();
    static BoundedGeometry *parseComposite(ParserContext &ctx);
};

#endif
