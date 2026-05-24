#ifndef __POV_AST_OBJECT_PARSER_H__
#define __POV_AST_OBJECT_PARSER_H__

#include "io/pov/ast/AstNodes.h"

class ParserContext;

class AstObjectParser {
  public:
    static AstSphereNode *parseSphere(ParserContext &ctx);
    static AstLightSourceNode *parseLightSource(ParserContext &ctx);
    static AstPlaneNode *parsePlane(ParserContext &ctx);
    static AstBoxNode *parseBox(ParserContext &ctx);
    static AstQuadricNode *parseQuadric(ParserContext &ctx);
    static AstBlobNode *parseBlob(ParserContext &ctx);
    static AstTriangleNode *parseTriangle(ParserContext &ctx);
    static AstSmoothTriangleNode *parseSmoothTriangle(ParserContext &ctx);
    static AstPolyNode *parsePoly(ParserContext &ctx, int knownOrder);
    static AstCsgNode *parseCsg(ParserContext &ctx, AstCsgOpKind op);
    static AstObjectNode *parseObject(ParserContext &ctx);
    static AstCompositeNode *parseComposite(ParserContext &ctx);

  private:
    static bool appendTransformOrFail(ParserContext &ctx, AstTransform *arr,
        int &count, AstTransformKind kind, const AstVector3 &v);
    static AstNode *parseShapeNode(ParserContext &ctx);
    static AstNode *parseShapeNodeFromToken(ParserContext &ctx, int tokenId);
};

#endif
