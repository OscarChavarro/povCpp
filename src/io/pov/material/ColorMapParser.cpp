#include "io/pov/material/ColorMapParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

RGBAColorPalette *
ColorMapParser::toRGBAColorPalette(PovColorMap *map)
{
    const int n = map->size();
    RGBAColorPalette * const palette = new RGBAColorPalette();

    if (n == 0) {
        delete map;
        return palette;
    }

    // Collect and sort spans by start value (canonical order)
    const PovColorMapSpan *sorted[20];
    for (int i = 0; i < n; i++) {
        sorted[i] = map->getSpanAt(i);
    }
    for (int i = 1; i < n; i++) {
        const PovColorMapSpan * const key = sorted[i];
        int j = i - 1;
        while (j >= 0 && sorted[j]->getStart() > key->getStart()) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }

    for (int i = 0; i < n; i++) {
        palette->addColorAt(sorted[i]->getStart(), sorted[i]->getStartColor());
        palette->addColorAt(sorted[i]->getEnd(),   sorted[i]->getEndColor());
    }

    delete map;
    return palette;
}

RGBAColorPalette *
ColorMapParser::parseColorMap(ParserContext &ctx)
{
    PovColorMap * const raw = new PovColorMap();

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    {
        bool exitFlag = false;
        while (!exitFlag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::LEFT_SQUARE_TOKEN: {
                const double spanStart = PrimitiveParser::parseFloat(ctx);
                const double spanEnd   = PrimitiveParser::parseFloat(ctx);

                ColorRgba sc(0.0, 0.0, 0.0, 0.0);
                ColorRgba ec(0.0, 0.0, 0.0, 0.0);
                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColor(&sc, ctx);

                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColor(&ec, ctx);

                raw->addSpan(spanStart, spanEnd, sc, ec);

                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_SQUARE_TOKEN, ctx);
                break;
            }
            case Tokenizer::RIGHT_CURLY_TOKEN:
                exitFlag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return ColorMapParser::toRGBAColorPalette(raw);
}
