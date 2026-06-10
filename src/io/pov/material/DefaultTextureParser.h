#ifndef __DEFAULT_TEXTURE_PARSER_H__
#define __DEFAULT_TEXTURE_PARSER_H__

class RenderFrame;
class ParserContext;

class DefaultTextureParser {
  public:
    static void parseDefault(RenderFrame *framePtr);
    static void parseDefault(RenderFrame *framePtr, ParserContext &ctx);
};

#endif
