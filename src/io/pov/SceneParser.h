#ifndef __SCENE_PARSER_H__
#define __SCENE_PARSER_H__

class RenderFrame;

class SceneParser {
  public:
    static void Parse(RenderFrame *framePtr);
    static void tokenInit();
    static void frameInit();
    static void parseFrame();
};

#endif
