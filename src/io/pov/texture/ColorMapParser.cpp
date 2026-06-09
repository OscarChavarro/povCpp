#include "io/pov/context/ParserContext.h"
#include "io/pov/texture/ColorMapParser.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"


RGBAColorPalette *
ColorMapParser::parseColorMap()
{
    ParserContext ctx;
    return ColorMapParser::parseColorMap(ctx);
}

RGBAColorPalette *
ColorMapParser::parseColorMap(ParserContext &ctx)
{
    RGBAColorPalette *newColorMap = new RGBAColorPalette();
    if (newColorMap == nullptr) {
        ParseErrorReporter::reportError("Not enough memory for color map.", ctx);
    }

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LEFT_SQUARE_TOKEN: {
                double spanStart = PrimitiveParser::parseFloat(ctx);
                double spanEnd   = PrimitiveParser::parseFloat(ctx);

                ColorRgba sc, ec;
                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColor(&sc, ctx);

                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColor(&ec, ctx);

                newColorMap->addSpan(spanStart, spanEnd, sc, ec);

                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_SQUARE_TOKEN, ctx);
                break;
            }
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return newColorMap;
}
