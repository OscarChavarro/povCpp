#ifndef __OBJECT_PARSER_H__
#define __OBJECT_PARSER_H__

#include "environment/geometry/elements/GeometryTypes.h"

class Geometry;
class SimpleBody;
class CSG;
class ParserContext;

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
