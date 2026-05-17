#include "io/pov/RenderSettingsParser.h"
#include "app/PovApp.h"
#include "io/pov/Parse.h"

extern double maxTraceLevel;

void
RenderSettingsParser::parseMaxTraceLevel()
{
    maxTraceLevel = PrimitiveParser::parseFloat();
}
