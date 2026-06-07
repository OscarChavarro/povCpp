#include "io/pov/context/ParserContext.h"
#include "io/pov/RenderSettingsParser.h"
#include "io/pov/PrimitiveParser.h"
#include "environment/material/RenderRuntimeState.h"


void
RenderSettingsParser::parseMaxTraceLevel()
{
    ParserContext ctx;
    RenderSettingsParser::parseMaxTraceLevel(ctx);
}

void
RenderSettingsParser::parseMaxTraceLevel(ParserContext &ctx)
{
    RenderRuntimeState::maxTraceLevel() = PrimitiveParser::parseFloat(ctx);
}
