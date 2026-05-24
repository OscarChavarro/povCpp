#include "io/pov/ParserContext.h"

#include "common/color/RGBAColorPaletteSpan.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "io/pov/ITokenStream.h"
#include "io/pov/RewindableTokenStream.h"
#include "io/pov/TokenizerTokenStream.h"
#include "processing/PolynomialConstants.h"

TokenizerTokenStream ParserContext::sDefaultTokenStream;
RewindableTokenStream ParserContext::sRewindableDefaultTokenStream(&ParserContext::sDefaultTokenStream);
RenderFrame *ParserContext::sSharedParsingFramePtr = nullptr;
RGBAColorPaletteSpan *ParserContext::sSharedConstructionMap = nullptr;
SymbolTable ParserContext::sSharedSymbols;
int ParserContext::sSharedDegenerateTriangles = 0;
ITokenStream *ParserContext::sForcedTokenStream = nullptr;

ParserContext::ParserContext()
{
    mTokenStream = (sForcedTokenStream != nullptr) ? sForcedTokenStream : &sDefaultTokenStream;
    mParsingFramePtr = &sSharedParsingFramePtr;
    mConstructionMap = &sSharedConstructionMap;
    mSymbols = &sSharedSymbols;
    mDegenerateTriangles = &sSharedDegenerateTriangles;
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

void
ParserContext::resetTokenStreamHistory()
{
    if (mTokenStream == &sRewindableDefaultTokenStream) {
        sRewindableDefaultTokenStream.clear();
    }
}

void
ParserContext::forceTokenStream(ITokenStream *tokenStream)
{
    sForcedTokenStream = tokenStream;
}

void
ParserContext::clearForcedTokenStream()
{
    sForcedTokenStream = nullptr;
}
