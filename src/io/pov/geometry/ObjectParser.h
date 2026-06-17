#ifndef __OBJECT_PARSER_H__
#define __OBJECT_PARSER_H__

#include "environment/geometry/elements/GeometryTypes.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/volume/compound/CSG.h"
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
    static CSG *parseCsg(GeometryTypes type);
    static CSG *parseCsg(GeometryTypes type, ParserContext &ctx);
};

#endif
