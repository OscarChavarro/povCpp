#include "io/pov/lexer/ITokenStream.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/context/TokenizerStream.h"

TokenizerStream ParserContext::sDefaultTokenStream;
RenderFrame *ParserContext::sSharedParsingFramePtr = nullptr;
SymbolTable ParserContext::sSharedSymbols;
int ParserContext::sSharedDegenerateTriangles = 0;
ITokenStream *ParserContext::sForcedTokenStream = nullptr;

ParserContext::ParserContext()
    : mTokenStream((sForcedTokenStream != nullptr) ? sForcedTokenStream : &sDefaultTokenStream),
      mParsingFramePtr(&sSharedParsingFramePtr),
      mSymbols(&sSharedSymbols),
      mDegenerateTriangles(&sSharedDegenerateTriangles)
{
}

const ReservedWord *
ParserContext::reservedWords()
{
    return mTokenStream->reservedWords();
}

TokenStruct &
ParserContext::token()
{
    return mTokenStream->token();
}

RenderFrame *&
ParserContext::parsingFrame()
{
    return *mParsingFramePtr;
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

int
ParserContext::findConstant()
{
    return symbols().findByIdentifierNumber(token().identifierNumber);
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
    // No-op: rewind/capture compatibility token stream removed from active pipeline.
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
#include "java/util/PriorityQueue.txx"
