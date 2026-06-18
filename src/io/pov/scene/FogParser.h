#ifndef __FOG_PARSER__
#define __FOG_PARSER__

#include "environment/scene/SceneFrame.h"
#include "io/pov/context/ParserContext.h"

class FogParser {
  public:
    static void parseFog(RenderFrame *framePtr);
    static void parseFog(RenderFrame *framePtr, ParserContext &ctx);
};

#endif
