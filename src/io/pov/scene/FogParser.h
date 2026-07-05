#ifndef __FOG_PARSER__
#define __FOG_PARSER__

#include "environment/scene/Scene.h"

class FogParser {
  public:
    static void parseFog(Scene *framePtr);
    static void parseFog(Scene *framePtr, ParserContext &ctx);
    static void parseFog(Scene &frame, ParserContext &ctx);
    static void parseFog(ColorRgba &fogColor, double &fogDistance, ParserContext &ctx);
};

#endif
