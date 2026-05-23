#include "io/pov/ParserContext.h"

#include "common/color/RGBAColorPaletteSpan.h"
#include "environment/scene/SceneFrame.h"
#include "io/pov/SceneConfigParser.h"
#include "processing/PolynomialConstants.h"

extern ReservedWord globalReservedWords[];
extern TokenStruct globalToken;
extern int termCounts[MAX_ORDER + 1];

static RenderFrame *parsingFramePtrInstance = nullptr;
static RGBAColorPaletteSpan *constructionMapInstance = nullptr;
static Constant constantsInstance[MAX_CONSTANTS];
static int numberOfConstantsInstance = 0;
static int degenerateTrianglesInstance = 0;

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
    return parsingFramePtrInstance;
}

RGBAColorPaletteSpan *&
ParserContext::constructionMap()
{
    return constructionMapInstance;
}

Constant *
ParserContext::constants()
{
    return constantsInstance;
}

int &
ParserContext::numberOfConstants()
{
    return numberOfConstantsInstance;
}

int &
ParserContext::degenerateTriangles()
{
    return degenerateTrianglesInstance;
}
