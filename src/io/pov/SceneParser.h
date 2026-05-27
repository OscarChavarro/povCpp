#ifndef __SCENE_PARSER_H__
#define __SCENE_PARSER_H__

class RenderFrame;
class ParserContext;

class SceneParser {
  public:
    static void Parse(RenderFrame *framePtr);
    static void Parse(RenderFrame *framePtr, ParserContext &ctx);
    static void tokenInit();
    static void tokenInit(ParserContext &ctx);
    static void frameInit();
    static void frameInit(ParserContext &ctx);
    static void parseFrame();
    static void parseFrame(ParserContext &ctx);
};

#endif
