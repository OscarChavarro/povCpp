#include "io/pov/context/TokenizerStream.h"

TokenizerStream::TokenizerStream(Tokenizer *tokenizer)
    : mTokenizer(tokenizer)
{
}

const ReservedWord *
TokenizerStream::reservedWords()
{
    return mTokenizer->reservedWords();
}

PovToken &
TokenizerStream::token()
{
    return mTokenizer->token();
}

void
TokenizerStream::getToken()
{
    mTokenizer->getToken();
}

void
TokenizerStream::ungetToken()
{
    mTokenizer->ungetToken();
}
