#ifndef __SCENE_BODY_PARSER__
#define __SCENE_BODY_PARSER__

#include "environment/scene/Scene.h"
#include "io/pov/context/ParserContext.h"

class SceneBodyParser {
  public:
    static void parseFrame(Scene *framePtr);
    static void parseFrame(Scene *framePtr, ParserContext &ctx);
};

#endif
