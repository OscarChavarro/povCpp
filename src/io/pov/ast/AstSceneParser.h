#ifndef __POV_AST_SCENE_PARSER_H__
#define __POV_AST_SCENE_PARSER_H__

class ParserContext;
class AstNode;
class AstDeclareNode;
class AstParsedSceneProgram;
class AstFogNode;
class AstCameraNode;
class AstMaxTraceLevelNode;
class AstDefaultTextureNode;

class AstSceneParser {
  public:
    static AstParsedSceneProgram *parseProgram(ParserContext &ctx);

  private:
    static bool isAstDeclareToken(int tokenId);
    static AstFogNode *parseFogNode(ParserContext &ctx);
    static AstCameraNode *parseCameraNode(ParserContext &ctx);
    static AstMaxTraceLevelNode *parseMaxTraceLevelNode(ParserContext &ctx);
    static AstDefaultTextureNode *parseDefaultTextureNode(ParserContext &ctx);
    static AstNode *parseRootNodeForToken(ParserContext &ctx, int tokenId);
    static AstDeclareNode *parseDeclareNode(ParserContext &ctx);
};

#endif
