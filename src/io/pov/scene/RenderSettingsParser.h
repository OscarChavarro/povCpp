#ifndef __RENDER_SETTINGS_PARSER__
#define __RENDER_SETTINGS_PARSER__

#include "io/pov/context/ParserContext.h"

class RenderSettingsParser {
  public:
    static void parseMaxTraceLevel(ParserContext &ctx);
};

#endif
