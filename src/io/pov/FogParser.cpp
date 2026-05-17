#include "io/pov/FogParser.h"
#include "io/pov/Parse.h"
#include "common/PovProto.h"
#include "render/RenderFrame.h"

extern TokenStruct globalToken;

void
FogParser::parseFog(RenderFrame *framePtr)
{
    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case COLOUR_TOKEN:
    PrimitiveParser::parseColour(&framePtr->Fog_Colour);
    break;

    case FLOAT_TOKEN:
    framePtr->Fog_Distance = globalToken.Token_Float;
    break;

    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
}
