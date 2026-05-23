#include "io/pov/mediaParser/ColorMapParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"

extern TokenStruct globalToken;
extern RGBAColorPaletteSpan *constructionMap;

RGBAColorPalette *
ColorMapParser::parseColorMap()
{
    static constexpr int MAX_ENTRIES = 20;
    RGBAColorPalette *newColourMap;
    int i;
    int j;

    newColourMap = new RGBAColorPalette;
    if (newColourMap == nullptr) {
        ParseErrorReporter::Error("Not enough memory for colour map.");
    }

    if (constructionMap == nullptr) {
        constructionMap = new RGBAColorPaletteSpan[MAX_ENTRIES];
        if (constructionMap == nullptr) {
            ParseErrorReporter::Error("Not enough memory for colour map.");
        }
    }

    i = 0;
    newColourMap->transparencyFlag = FALSE;
    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.tokenId) {
            case LEFT_SQUARE_TOKEN:
                constructionMap[i].start = PrimitiveParser::parseFloat();
                constructionMap[i].end = PrimitiveParser::parseFloat();

                ParseHelpers::getExpectedToken(COLOUR_TOKEN);
                PrimitiveParser::parseColour(
                    &(constructionMap[i].startColour));
                if (constructionMap[i].startColour.Alpha != 0.0) {
                    newColourMap->transparencyFlag = TRUE;
                }

                ParseHelpers::getExpectedToken(COLOUR_TOKEN);
                PrimitiveParser::parseColour(&(constructionMap[i].endColour));
                if (constructionMap[i].endColour.Alpha != 0.0) {
                    newColourMap->transparencyFlag = TRUE;
                }

                i++;
                if (i > MAX_ENTRIES) {
                    ParseErrorReporter::Error("Colour_Map too long.");
                }
                ParseHelpers::getExpectedToken(RIGHT_SQUARE_TOKEN);
                break;

            case RIGHT_CURLY_TOKEN:
                newColourMap->numberOfEntries = i;

                newColourMap->Colour_Map_Entries = new RGBAColorPaletteSpan[i];
                if (newColourMap == nullptr) {
                    ParseErrorReporter::Error(
                        "Not enough memory for colour map.");
                }

                for (j = 0; j < i; j++) {
                    newColourMap->Colour_Map_Entries[j] = constructionMap[j];
                }

                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
                break;
            }
        }
    }

    return (newColourMap);
}
