#include "io/pov/ParserContext.h"
#include "render/RenderEngine.h"
#include "io/pov/RenderSettingsParser.h"
#include "io/pov/Parse.h"


void
RenderSettingsParser::parseMaxTraceLevel()
{
    ParserContext ctx;
    RenderSettingsParser::parseMaxTraceLevel(ctx);
}

void
RenderSettingsParser::parseMaxTraceLevel(ParserContext &ctx)
{
    RenderEngine::maxTraceLevel() = PrimitiveParser::parseFloat(ctx);
}
