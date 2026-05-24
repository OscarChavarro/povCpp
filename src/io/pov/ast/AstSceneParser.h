#ifndef __POV_AST_SCENE_PARSER_H__
#define __POV_AST_SCENE_PARSER_H__

class ParserContext;
class AstScene;

class AstSceneParser {
  public:
    static AstScene *parseScene(ParserContext &ctx);
};

#endif
