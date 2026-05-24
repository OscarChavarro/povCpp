#include "io/pov/ParserContext.h"

#include "common/color/RGBAColorPaletteSpan.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "io/pov/ITokenStream.h"
#include "io/pov/TokenizerTokenStream.h"
#include "processing/PolynomialConstants.h"

static TokenizerTokenStream defaultTokenStream;
static RenderFrame *sharedParsingFramePtr = nullptr;
static RGBAColorPaletteSpan *sharedConstructionMap = nullptr;
static SymbolTable sharedSymbols;
static int sharedDegenerateTriangles = 0;

ParserContext::ParserContext()
{
    mTokenStream = &defaultTokenStream;
    mParsingFramePtr = &sharedParsingFramePtr;
    mConstructionMap = &sharedConstructionMap;
    mSymbols = &sharedSymbols;
    mDegenerateTriangles = &sharedDegenerateTriangles;
}

ReservedWord *
ParserContext::reservedWords()
{
    return mTokenStream->reservedWords();
}

TokenStruct &
ParserContext::token()
{
    return mTokenStream->token();
}

int *
ParserContext::termCounts()
{
    return PolynomialShape::termCounts();
}

RenderFrame *&
ParserContext::parsingFrame()
{
    return *mParsingFramePtr;
}

RGBAColorPaletteSpan *&
ParserContext::constructionMap()
{
    return *mConstructionMap;
}

Constant *
ParserContext::constants()
{
    return mSymbols->data();
}

int &
ParserContext::numberOfConstants()
{
    return mSymbols->size();
}

SymbolTable &
ParserContext::symbols()
{
    return *mSymbols;
}

int &
ParserContext::degenerateTriangles()
{
    return *mDegenerateTriangles;
}

ITokenStream &
ParserContext::tokenStream()
{
    return *mTokenStream;
}

void
ParserContext::setTokenStream(ITokenStream *tokenStream)
{
    if (tokenStream != nullptr) {
        mTokenStream = tokenStream;
    }
}
