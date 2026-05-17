#include "RenderSettingsParser.h"
#include "Parse.h"
#include "common/PovProto.h"

extern double maxTraceLevel;

void
RenderSettingsParser::parseMaxTraceLevel()
{
    maxTraceLevel = PrimitiveParser::parseFloat();
}
