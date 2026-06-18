#ifndef __SCENE_PARSER__
#define __SCENE_PARSER__

#include "environment/scene/Scene.h"
#include "io/pov/context/ParserContext.h"

class SceneParser {
  public:
    static void parse(Scene *framePtr);
    static void parse(Scene *framePtr, ParserContext &ctx);
    static void tokenInit();
    static void tokenInit(ParserContext &ctx);
    static void frameInit();
    static void frameInit(ParserContext &ctx);
    static void parseFrame();
    static void parseFrame(ParserContext &ctx);

  private:
    static void postProcessPhase(ParserContext &ctx);
};

#endif
