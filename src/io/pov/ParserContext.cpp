#include "io/pov/ParserContext.h"

#include "common/color/RGBAColorPaletteSpan.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/scene/SceneFrame.h"
#include "io/Tokenizer.h"
#include "io/pov/SceneConfigParser.h"
#include "processing/PolynomialConstants.h"

static RenderFrame *parsingFramePtrInstance = nullptr;
static RGBAColorPaletteSpan *constructionMapInstance = nullptr;
static Constant constantsInstance[MAX_CONSTANTS];
static int numberOfConstantsInstance = 0;
static int degenerateTrianglesInstance = 0;

ReservedWord *
ParserContext::reservedWords()
{
    return Tokenizer::reservedWords();
}

TokenStruct &
ParserContext::token()
{
    return Tokenizer::token();
}

int *
ParserContext::termCounts()
{
    return PolynomialShape::termCounts();
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
