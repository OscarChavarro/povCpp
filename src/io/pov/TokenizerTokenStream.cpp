#include "io/pov/TokenizerTokenStream.h"

#include "io/Tokenizer.h"

ReservedWord *
TokenizerTokenStream::reservedWords()
{
    return Tokenizer::reservedWords();
}

TokenStruct &
TokenizerTokenStream::token()
{
    return Tokenizer::token();
}

void
TokenizerTokenStream::getToken()
{
    Tokenizer::getToken();
}

void
TokenizerTokenStream::ungetToken()
{
    Tokenizer::ungetToken();
}
