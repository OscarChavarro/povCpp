#ifndef __SCENE_PARSER_H__
#define __SCENE_PARSER_H__

class Frame;

class SceneParser {
  public:
    static void Parse(Frame *framePtr);
    static void tokenInit();
    static void frameInit();
    static void parseFrame();
};

#endif
