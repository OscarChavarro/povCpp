#ifndef __SCENE_FRAME_PARSER_H__
#define __SCENE_FRAME_PARSER_H__

class RenderFrame;
class ParserContext;

class SceneFrameParser {
  public:
    static void parseFrame(RenderFrame *framePtr);
    static void parseFrame(RenderFrame *framePtr, ParserContext &ctx);
};

#endif
