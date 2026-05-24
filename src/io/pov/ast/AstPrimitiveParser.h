#ifndef __POV_AST_PRIMITIVE_PARSER_H__
#define __POV_AST_PRIMITIVE_PARSER_H__

#include "io/pov/ast/AstNodes.h"

class ParserContext;

class AstPrimitiveParser {
  public:
    static AstVector3 parseVector(ParserContext &ctx);
    static AstColor parseColour(ParserContext &ctx);
    static double parseFloat(ParserContext &ctx);
};

#endif
