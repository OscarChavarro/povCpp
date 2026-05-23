#include "io/pov/ParserContext.h"

#include "common/color/RGBAColorPaletteSpan.h"
#include "environment/scene/SceneFrame.h"
#include "io/pov/SceneConfigParser.h"
#include "processing/PolynomialConstants.h"

extern ReservedWord globalReservedWords[];
extern TokenStruct globalToken;
extern int termCounts[MAX_ORDER + 1];
extern RenderFrame *parsingFramePtr;
extern RGBAColorPaletteSpan *constructionMap;
extern Constant constants[MAX_CONSTANTS];
extern int numberOfConstants;
extern int degenerateTriangles;

ReservedWord *
ParserContext::reservedWords()
{
    return globalReservedWords;
}

TokenStruct &
ParserContext::token()
{
    return globalToken;
}

int *
ParserContext::termCounts()
{
    return ::termCounts;
}

RenderFrame *&
ParserContext::parsingFrame()
{
    return parsingFramePtr;
}

RGBAColorPaletteSpan *&
ParserContext::constructionMap()
{
    return ::constructionMap;
}

Constant *
ParserContext::constants()
{
    return ::constants;
}

int &
ParserContext::numberOfConstants()
{
    return ::numberOfConstants;
}

int &
ParserContext::degenerateTriangles()
{
    return ::degenerateTriangles;
}
