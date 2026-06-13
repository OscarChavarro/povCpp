#ifndef __FOG_PARSER_H__
#define __FOG_PARSER_H__

#include "environment/scene/SceneFrame.h"
#include "io/pov/context/ParserContext.h"

class FogParser {
  public:
    static void parseFog(RenderFrame *framePtr);
    static void parseFog(RenderFrame *framePtr, ParserContext &ctx);
};

#endif
