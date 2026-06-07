#include "io/pov/lexer/TokenizerStream.h"

#include "io/pov/lexer/Tokenizer.h"

ReservedWord *
TokenizerStream::reservedWords()
{
    return Tokenizer::reservedWords();
}

TokenStruct &
TokenizerStream::token()
{
    return Tokenizer::token();
}

void
TokenizerStream::getToken()
{
    Tokenizer::getToken();
}

void
TokenizerStream::ungetToken()
{
    Tokenizer::ungetToken();
}
