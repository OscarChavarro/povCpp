#ifndef __TOKENIZER_TOKEN_STREAM_H__
#define __TOKENIZER_TOKEN_STREAM_H__

#include "io/pov/ITokenStream.h"

class TokenizerTokenStream : public ITokenStream {
  public:
    ReservedWord *reservedWords() override;
    TokenStruct &token() override;
    void getToken() override;
    void ungetToken() override;
};

#endif
