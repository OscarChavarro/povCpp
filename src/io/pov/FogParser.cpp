#include "io/pov/ParserContext.h"
#include "io/pov/FogParser.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
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
    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::COLOUR_TOKEN:
                PrimitiveParser::parseColour(&framePtr->fogColour, ctx);
                break;

            case Tokenizer::FLOAT_TOKEN:
                framePtr->fogDistance = ctx.token().tokenFloat;
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
}
