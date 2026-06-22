#include "io/context/RenderRuntimeState.h"

#include "io/pov/context/ParserContext.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "io/pov/scene/RenderSettingsParser.h"

void
RenderSettingsParser::parseMaxTraceLevel(ParserContext &ctx)
{
    ctx.getRuntimeState()->getMaxTraceLevel() = PrimitiveParser::parseFloat(ctx);
}
