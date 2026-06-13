#ifndef __SCENE_PARSER_H__
#define __SCENE_PARSER_H__

#include "environment/scene/SceneFrame.h"
#include "io/pov/context/ParserContext.h"

class SceneParser {
  public:
    static void parse(RenderFrame *framePtr);
    static void parse(RenderFrame *framePtr, ParserContext &ctx);
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
