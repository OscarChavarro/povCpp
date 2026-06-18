#ifndef __TOKENIZER_TOKEN_STREAM_H__
#define __TOKENIZER_TOKEN_STREAM_H__

#include "io/pov/lexer/ITokenStream.h"

class TokenizerStream : public ITokenStream {
  public:
    const ReservedWord *reservedWords() override;
    PovToken &token() override;
    void getToken() override;
    void ungetToken() override;
};

#endif
