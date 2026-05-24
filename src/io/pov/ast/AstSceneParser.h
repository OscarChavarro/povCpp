#ifndef __POV_AST_SCENE_PARSER_H__
#define __POV_AST_SCENE_PARSER_H__

class ParserContext;
class AstScene;
class AstNode;
class AstDeclareNode;
class RenderFrame;

class AstSceneParser {
  public:
    static AstScene *parseScene(ParserContext &ctx, RenderFrame *framePtr);

  private:
    static AstNode *parseRootNodeForToken(ParserContext &ctx, int tokenId);
    static AstDeclareNode *parseDeclareNode(ParserContext &ctx);
};

#endif
