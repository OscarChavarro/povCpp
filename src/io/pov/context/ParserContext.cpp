#include "java/util/PriorityQueue.txx"

#include "environment/material/RendererConfiguration.h"
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
ParserContext::resetTokenStreamHistory()
{
    // No-op: rewind/capture compatibility token stream removed from active pipeline.
}

bool
ParserContext::writesVerboseErrors() const
{
    return mReportingConfig != nullptr &&
        mReportingConfig->hasOptionFlags(RenderingConfiguration::VERBOSE_FILE);
}

const char *
ParserContext::statFileName() const
{
    return mReportingConfig->getStatFileName();
}
