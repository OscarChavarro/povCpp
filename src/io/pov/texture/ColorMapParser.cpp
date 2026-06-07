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
    RGBAColorPalette *newColourMap;
    int i;
    int j;

    newColourMap = new RGBAColorPalette;
    if (newColourMap == nullptr) {
        ParseErrorReporter::reportError("Not enough memory for colour map.", ctx);
    }

    if (ctx.constructionMap() == nullptr) {
        ctx.constructionMap() = new RGBAColorPaletteSpan[MAX_ENTRIES];
        if (ctx.constructionMap() == nullptr) {
            ParseErrorReporter::reportError("Not enough memory for colour map.", ctx);
        }
    }

    i = 0;
    newColourMap->transparencyFlag = false;
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
                PrimitiveParser::parseColour(
                    &(ctx.constructionMap()[i].startColour), ctx);
                if (ctx.constructionMap()[i].startColour.Alpha != 0.0) {
                    newColourMap->transparencyFlag = true;
                }

                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColour(&(ctx.constructionMap()[i].endColour), ctx);
                if (ctx.constructionMap()[i].endColour.Alpha != 0.0) {
                    newColourMap->transparencyFlag = true;
                }

                i++;
                if (i > MAX_ENTRIES) {
                    ParseErrorReporter::reportError("Colour_Map too long.", ctx);
                }
                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_SQUARE_TOKEN, ctx);
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                newColourMap->numberOfEntries = i;

                newColourMap->Colour_Map_Entries = new RGBAColorPaletteSpan[i];
                if (newColourMap == nullptr) {
                    ParseErrorReporter::reportError(
                        "Not enough memory for colour map.", ctx);
                }

                for (j = 0; j < i; j++) {
                    newColourMap->Colour_Map_Entries[j] = ctx.constructionMap()[j];
                }

                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return (newColourMap);
}
