#include "io/pov/context/ParserContext.h"
#include "io/pov/scene/FogParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
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
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::COLOUR_TOKEN:
                PrimitiveParser::parseColor(&framePtr->fogColor, ctx);
                break;

            case Tokenizer::FLOAT_TOKEN:
                framePtr->fogDistance = ctx.token().tokenFloat;
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
}
