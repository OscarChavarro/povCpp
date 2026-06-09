#include "io/pov/context/ParserContext.h"
#include "io/pov/texture/ColorMapParser.h"
#include "common/color/RGBAColorPalette.h"
#include "common/color/RGBAColorPaletteSpan.h"
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
    static constexpr int MAX_ENTRIES = 20;
    RGBAColorPalette *newColorMap;
    int i;
    int j;

    newColorMap = new RGBAColorPalette;
    if (newColorMap == nullptr) {
        ParseErrorReporter::reportError("Not enough memory for color map.", ctx);
    }

    if (ctx.constructionMap() == nullptr) {
        ctx.constructionMap() = new RGBAColorPaletteSpan[MAX_ENTRIES];
        if (ctx.constructionMap() == nullptr) {
            ParseErrorReporter::reportError("Not enough memory for color map.", ctx);
        }
    }

    i = 0;
    newColorMap->transparencyFlag = false;
    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LEFT_SQUARE_TOKEN:
                ctx.constructionMap()[i].start = PrimitiveParser::parseFloat(ctx);
                ctx.constructionMap()[i].end = PrimitiveParser::parseFloat(ctx);

                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColor(
                    &(ctx.constructionMap()[i].startColor), ctx);
                if (ctx.constructionMap()[i].startColor.getA() != 0.0) {
                    newColorMap->transparencyFlag = true;
                }

                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColor(&(ctx.constructionMap()[i].endColor), ctx);
                if (ctx.constructionMap()[i].endColor.getA() != 0.0) {
                    newColorMap->transparencyFlag = true;
                }

                i++;
                if (i > MAX_ENTRIES) {
                    ParseErrorReporter::reportError("Colour_Map too long.", ctx);
                }
                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_SQUARE_TOKEN, ctx);
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                newColorMap->numberOfEntries = i;

                newColorMap->colorMapEntries = new RGBAColorPaletteSpan[i];
                if (newColorMap == nullptr) {
                    ParseErrorReporter::reportError(
                        "Not enough memory for color map.", ctx);
                }

                for (j = 0; j < i; j++) {
                    newColorMap->colorMapEntries[j] = ctx.constructionMap()[j];
                }

                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return (newColorMap);
}
