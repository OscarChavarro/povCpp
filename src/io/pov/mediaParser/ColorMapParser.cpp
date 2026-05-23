#include "io/pov/ParserContext.h"
#include "io/pov/mediaParser/ColorMapParser.h"
#include "common/color/RGBAColorPalette.h"
#include "common/color/RGBAColorPaletteSpan.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"


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
        ParseErrorReporter::Error("Not enough memory for colour map.", ctx);
    }

    if (ctx.constructionMap() == nullptr) {
        ctx.constructionMap() = new RGBAColorPaletteSpan[MAX_ENTRIES];
        if (ctx.constructionMap() == nullptr) {
            ParseErrorReporter::Error("Not enough memory for colour map.", ctx);
        }
    }

    i = 0;
    newColourMap->transparencyFlag = FALSE;
    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case LEFT_SQUARE_TOKEN:
                ctx.constructionMap()[i].start = PrimitiveParser::parseFloat(ctx);
                ctx.constructionMap()[i].end = PrimitiveParser::parseFloat(ctx);

                ParseHelpers::getExpectedToken(COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColour(
                    &(ctx.constructionMap()[i].startColour), ctx);
                if (ctx.constructionMap()[i].startColour.Alpha != 0.0) {
                    newColourMap->transparencyFlag = TRUE;
                }

                ParseHelpers::getExpectedToken(COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColour(&(ctx.constructionMap()[i].endColour), ctx);
                if (ctx.constructionMap()[i].endColour.Alpha != 0.0) {
                    newColourMap->transparencyFlag = TRUE;
                }

                i++;
                if (i > MAX_ENTRIES) {
                    ParseErrorReporter::Error("Colour_Map too long.", ctx);
                }
                ParseHelpers::getExpectedToken(RIGHT_SQUARE_TOKEN, ctx);
                break;

            case RIGHT_CURLY_TOKEN:
                newColourMap->numberOfEntries = i;

                newColourMap->Colour_Map_Entries = new RGBAColorPaletteSpan[i];
                if (newColourMap == nullptr) {
                    ParseErrorReporter::Error(
                        "Not enough memory for colour map.", ctx);
                }

                for (j = 0; j < i; j++) {
                    newColourMap->Colour_Map_Entries[j] = ctx.constructionMap()[j];
                }

                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return (newColourMap);
}
