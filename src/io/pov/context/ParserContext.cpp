#include "java/util/PriorityQueue.txx"

#include "io/pov/context/ParserContext.h"
#include "io/pov/context/TokenizerStream.h"
#include "io/pov/lexer/ITokenStream.h"

ParserContext::ParserContext()
    : mTokenizer(),
      mDefaultTokenStream(&mTokenizer),
      mSharedSymbols(),
      mSharedDegenerateTriangles(0),
      mForcedTokenStream(nullptr),
      mTokenStream(&mDefaultTokenStream),
      mSymbols(&mSharedSymbols),
      mDegenerateTriangles(&mSharedDegenerateTriangles),
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
    mForcedTokenStream = tokenStream;
    mTokenStream = (mForcedTokenStream != nullptr) ? mForcedTokenStream : &mDefaultTokenStream;
}

void
ParserContext::clearForcedTokenStream()
{
    mForcedTokenStream = nullptr;
    mTokenStream = &mDefaultTokenStream;
}
