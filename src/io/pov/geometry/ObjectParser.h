#ifndef __OBJECT_PARSER_H__
#define __OBJECT_PARSER_H__

#include "environment/geometry/elements/GeometryTypes.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/SimpleBody.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "io/pov/context/ParserContext.h"

class ObjectParser {
  public:
    static Geometry *parseShape();
    static Geometry *parseShape(ParserContext &ctx);
    static SimpleBody *parseObject();
    static SimpleBody *parseObject(ParserContext &ctx);
    static SimpleBody *parseComposite();
    static SimpleBody *parseComposite(ParserContext &ctx);
    static CSG *parseCsg(GeometryTypes type);
    static CSG *parseCsg(GeometryTypes type, ParserContext &ctx);
};

#endif
