#ifndef __DEFAULT_TEXTURE_PARSER__
#define __DEFAULT_TEXTURE_PARSER__

#include "environment/scene/Scene.h"
#include "io/pov/context/ParserContext.h"

class DefaultTextureParser {
  public:
    static void parseDefault(Scene *framePtr);
    static void parseDefault(Scene *framePtr, ParserContext &ctx);
};

#endif
