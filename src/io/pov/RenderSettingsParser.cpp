#include "io/pov/RenderSettingsParser.h"
#include "io/pov/Parse.h"
#include "app/PovApp.h"

extern double maxTraceLevel;

void
RenderSettingsParser::parseMaxTraceLevel()
{
    maxTraceLevel = PrimitiveParser::parseFloat();
}
