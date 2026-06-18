#include "io/pov/context/TokenizerStream.h"
#include "io/pov/lexer/Tokenizer.h"

const ReservedWord *
TokenizerStream::reservedWords()
{
    return Tokenizer::reservedWords();
}

PovToken &
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
