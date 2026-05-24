#include "io/pov/RewindableTokenStream.h"

RewindableTokenStream::RewindableTokenStream()
{
    mSource = nullptr;
    mCursor = 0;
    mCurrentToken = TokenStruct();
}

RewindableTokenStream::RewindableTokenStream(ITokenStream *source)
{
    mSource = source;
    mCursor = 0;
    if (mSource != nullptr) {
        mCurrentToken = mSource->token();
    } else {
        mCurrentToken = TokenStruct();
    }
}

void
RewindableTokenStream::setSource(ITokenStream *source)
{
    mSource = source;
    clear();
}

void
RewindableTokenStream::clear()
{
    mHistory.clear();
    mCursor = 0;
    if (mSource != nullptr) {
        mCurrentToken = mSource->token();
    }
}

ReservedWord *
RewindableTokenStream::reservedWords()
{
    return mSource->reservedWords();
}

TokenStruct &
RewindableTokenStream::token()
{
    return mCurrentToken;
}

void
RewindableTokenStream::getToken()
{
    if (mCursor < mHistory.size()) {
        mCurrentToken = mHistory[mCursor];
        mCursor++;
        return;
    }

    mSource->getToken();
    mCurrentToken = mSource->token();
    mHistory.push_back(mCurrentToken);
    mCursor = mHistory.size();
}

void
RewindableTokenStream::ungetToken()
{
    if (mCursor == 0) {
        return;
    }

    mCursor--;
    if (mCursor > 0) {
        mCurrentToken = mHistory[mCursor - 1];
    }
}

bool
RewindableTokenStream::canRewind() const
{
    return true;
}

int
RewindableTokenStream::mark()
{
    return (int)mCursor;
}

bool
RewindableTokenStream::rewind(int marker)
{
    if (marker < 0 || (std::size_t)marker > mHistory.size()) {
        return false;
    }
    mCursor = (std::size_t)marker;
    if (mCursor > 0) {
        mCurrentToken = mHistory[mCursor - 1];
    }
    return true;
}
