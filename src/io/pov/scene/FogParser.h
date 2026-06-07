#ifndef __FOG_PARSER_H__
#define __FOG_PARSER_H__

class RenderFrame;
class ParserContext;

class FogParser {
  public:
    static void parseFog(RenderFrame *framePtr);
    static void parseFog(RenderFrame *framePtr, ParserContext &ctx);
};

#endif
