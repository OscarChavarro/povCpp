#include "java/util/PriorityQueue.txx"

#include "io/pov/context/ParserContext.h"
#include "io/pov/context/TokenizerStream.h"
#include "io/pov/lexer/ITokenStream.h"

TokenizerStream ParserContext::sDefaultTokenStream;
SymbolTable ParserContext::sSharedSymbols;
int ParserContext::sSharedDegenerateTriangles = 0;
ITokenStream *ParserContext::sForcedTokenStream = nullptr;

ParserContext::ParserContext()
    : mTokenStream((sForcedTokenStream != nullptr) ? sForcedTokenStream : &sDefaultTokenStream),
      mSymbols(&sSharedSymbols),
      mDegenerateTriangles(&sSharedDegenerateTriangles),
      mDefaultTexture(nullptr),
      mReportingConfig(nullptr),
      mRuntimeState(nullptr)
{
}

const ReservedWord *
ParserContext::reservedWords()
{
    return mTokenStream->reservedWords();
}

PovToken &
ParserContext::token()
{
    return mTokenStream->token();
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
    return symbols().findByIdentifierNumber(token().getIdentifierNumber());
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
