#ifndef __SCENE_BODY_PARSER__
#define __SCENE_BODY_PARSER__


class SceneBodyParser {
  public:
    static void parseFrame(Scene *framePtr);
    static void parseFrame(Scene *framePtr, ParserContext &ctx);
};

#endif
