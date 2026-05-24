#ifndef __POV_AST_SCENE_BUILDER_H__
#define __POV_AST_SCENE_BUILDER_H__

class AstScene;
class ParserContext;
class RenderFrame;

class AstSceneBuilder {
  public:
    static void build(const AstScene &scene, RenderFrame *framePtr, ParserContext &ctx);
};

#endif
