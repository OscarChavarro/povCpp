#include "common/RenderRuntimeState.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "io/pov/scene/RenderSettingsParser.h"


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
#include "common/dataStructures/PriorityQueue.txx"
