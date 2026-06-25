#ifndef __I_TOKEN_STREAM__
#define __I_TOKEN_STREAM__

#include "io/pov/lexer/ReservedWord.h"
#include "io/pov/lexer/PovToken.h"

class ITokenStream {
  public:
    virtual ~ITokenStream() {}
    virtual const ReservedWord *reservedWords() = 0;
    virtual PovToken &token() = 0;
    virtual void getToken() = 0;
    virtual void ungetToken() = 0;
};

#endif
