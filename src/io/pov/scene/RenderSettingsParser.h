#ifndef __RENDER_SETTINGS_PARSER_H__
#define __RENDER_SETTINGS_PARSER_H__

#include "io/pov/context/ParserContext.h"

class RenderSettingsParser {
  public:
    static void parseMaxTraceLevel();
    static void parseMaxTraceLevel(ParserContext &ctx);
};

#endif
