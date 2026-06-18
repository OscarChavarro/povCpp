#ifndef __DEFAULT_TEXTURE_PARSER__
#define __DEFAULT_TEXTURE_PARSER__

#include "environment/scene/SceneFrame.h"
#include "io/pov/context/ParserContext.h"

class DefaultTextureParser {
  public:
    static void parseDefault(RenderFrame *framePtr);
    static void parseDefault(RenderFrame *framePtr, ParserContext &ctx);
};

#endif
