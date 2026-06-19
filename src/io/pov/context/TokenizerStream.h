#ifndef __TOKENIZER_STREAM__
#define __TOKENIZER_STREAM__

#include "io/pov/lexer/Tokenizer.h"
#include "io/pov/lexer/ITokenStream.h"

class TokenizerStream : public ITokenStream {
  public:
    explicit TokenizerStream(Tokenizer *tokenizer);
    const ReservedWord *reservedWords() override;
    PovToken &token() override;
    void getToken() override;
    void ungetToken() override;

  private:
    Tokenizer *mTokenizer;
};

#endif
