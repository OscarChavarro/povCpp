#ifndef __SCENE_FRAME_PARSER__
#define __SCENE_FRAME_PARSER__

#include "environment/scene/SceneFrame.h"
#include "io/pov/context/ParserContext.h"

class SceneFrameParser {
  public:
    static void parseFrame(RenderFrame *framePtr);
    static void parseFrame(RenderFrame *framePtr, ParserContext &ctx);
};

#endif
