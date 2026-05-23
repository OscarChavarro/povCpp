#include "io/pov/FogParser.h"
#include "io/pov/Parse.h"
#include "environment/scene/SceneFrame.h"

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
            switch (globalToken.tokenId) {
            case COLOUR_TOKEN:
                PrimitiveParser::parseColour(&framePtr->fogColour);
                break;

            case FLOAT_TOKEN:
                framePtr->fogDistance = globalToken.tokenFloat;
                break;

            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
                break;
            }
        }
    }
}
