#include "io/pov/ParserContext.h"
#include "io/pov/FogParser.h"
#include "io/pov/Parse.h"
#include "environment/scene/SceneFrame.h"


void
FogParser::parseFog(RenderFrame *framePtr)
{
    ParserContext ctx;
    FogParser::parseFog(framePtr, ctx);
}

void
FogParser::parseFog(RenderFrame *framePtr, ParserContext &ctx)
{
    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case COLOUR_TOKEN:
                PrimitiveParser::parseColour(&framePtr->fogColour, ctx);
                break;

            case FLOAT_TOKEN:
                framePtr->fogDistance = ctx.token().tokenFloat;
                break;

            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
}
