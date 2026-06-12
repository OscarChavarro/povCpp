#include "io/pov/lexer/Tokenizer.h"
#include "io/pov/context/TokenizerStream.h"

const ReservedWord *
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
