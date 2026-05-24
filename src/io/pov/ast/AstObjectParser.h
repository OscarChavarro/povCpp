#ifndef __POV_AST_OBJECT_PARSER_H__
#define __POV_AST_OBJECT_PARSER_H__

#include "io/pov/ast/AstNodes.h"

class ParserContext;

class AstObjectParser {
  public:
    static AstSphereNode *parseSphere(ParserContext &ctx);
    static AstLightSourceNode *parseLightSource(ParserContext &ctx);
    static AstCsgNode *parseCsg(ParserContext &ctx, AstCsgOpKind op);
    static AstObjectNode *parseObject(ParserContext &ctx);
    static AstCompositeNode *parseComposite(ParserContext &ctx);
};

#endif
