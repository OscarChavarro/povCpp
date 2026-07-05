

#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "io/pov/scene/FogParser.h"
#include "java/util/PriorityQueue.txx"


void
FogParser::parseFog(Scene *framePtr)
{
    ParserContext ctx;
    FogParser::parseFog(*framePtr, ctx);
}

void
FogParser::parseFog(Scene *framePtr, ParserContext &ctx)
{
    FogParser::parseFog(*framePtr, ctx);
}

void
FogParser::parseFog(Scene &frame, ParserContext &ctx)
{
    ColorRgba fogColor = frame.getFogColor();
    double fogDistance = frame.getFogDistance();
    FogParser::parseFog(fogColor, fogDistance, ctx);
    frame.setFog(fogColor, fogDistance);
}

void
FogParser::parseFog(ColorRgba &fogColor, double &fogDistance, ParserContext &ctx)
{
    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::COLOUR_TOKEN:
                PrimitiveParser::parseColor(&fogColor, ctx);
                break;

            case Tokenizer::FLOAT_TOKEN:
                fogDistance = ctx.token().getTokenFloat();
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
