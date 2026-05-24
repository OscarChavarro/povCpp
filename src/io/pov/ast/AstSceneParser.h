#ifndef __POV_AST_SCENE_PARSER_H__
#define __POV_AST_SCENE_PARSER_H__

class ParserContext;
class AstNode;
class AstDeclareNode;
class AstParsedSceneProgram;

class AstSceneParser {
  public:
    static AstParsedSceneProgram *parseProgram(ParserContext &ctx);

  private:
    static AstNode *parseRootNodeForToken(ParserContext &ctx, int tokenId);
    static AstDeclareNode *parseDeclareNode(ParserContext &ctx);
};

#endif
