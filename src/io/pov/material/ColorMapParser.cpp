#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"

#include "io/pov/context/ParserContext.h"
#include "io/pov/material/ColorMapParser.h"
#include "io/pov/material/PovColorMap.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

// Convert a PovColorMap to an RGBAColorPalette with explicit stop positions.
// Spans are sorted canonically by start value. Each span contributes its
// startColor (at span.start) and endColor (at span.end). For contiguous spans
// (the normal case) the endColor of span[i] and startColor of span[i+1] share
// the same position — only one stop is emitted, using span[i].endColor.
// Gaps between non-adjacent spans are left uncovered; evalLinear returns zero
// for t values that fall in a gap, matching original PovColorMap behaviour.
static RGBAColorPalette* toRGBAColorPalette(PovColorMap* map) {
    const int n = map->size();
    RGBAColorPalette * const palette = new RGBAColorPalette();

    if (n == 0) {
        delete map;
        return palette;
    }

    // Collect and sort spans by start value (canonical order)
    const PovColorMap::Span* sorted[20];
    for (int i = 0; i < n; i++) {
        sorted[i] = map->getSpanAt(i);
    }
    for (int i = 1; i < n; i++) {
        const PovColorMap::Span * const key = sorted[i];
        int j = i - 1;
        while (j >= 0 && sorted[j]->start > key->start) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }

    // Build stops: for each span emit startColor at span.start and endColor at
    // span.end. At shared boundaries between adjacent spans both the endColor
    // of span[i] and the startColor of span[i+1] are emitted at the same
    // position — this creates a zero-width pair that evalLinear skips, but
    // preserves the correct colour on each side of any discontinuity.
    for (int i = 0; i < n; i++) {
        palette->addColorAt(sorted[i]->start, sorted[i]->startColor);
        palette->addColorAt(sorted[i]->end,   sorted[i]->endColor);
    }

    delete map;
    return palette;
}

RGBAColorPalette *
ColorMapParser::parseColorMap()
{
    ParserContext ctx;
    return ColorMapParser::parseColorMap(ctx);
}

RGBAColorPalette *
ColorMapParser::parseColorMap(ParserContext &ctx)
{
    PovColorMap * const raw = new PovColorMap();
    if (raw == nullptr) {
        ParseErrorReporter::reportError("Not enough memory for color map.", ctx);
    }

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LEFT_SQUARE_TOKEN: {
                const double spanStart = PrimitiveParser::parseFloat(ctx);
                const double spanEnd   = PrimitiveParser::parseFloat(ctx);

                ColorRgba sc, ec;
                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColor(&sc, ctx);

                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColor(&ec, ctx);

                raw->addSpan(spanStart, spanEnd, sc, ec);

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

    return toRGBAColorPalette(raw);
}
