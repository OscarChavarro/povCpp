#ifndef __RENDER_SETTINGS_PARSER_H__
#define __RENDER_SETTINGS_PARSER_H__

class ParserContext;

class RenderSettingsParser {
  public:
    static void parseMaxTraceLevel();
    static void parseMaxTraceLevel(ParserContext &ctx);
};

#endif
