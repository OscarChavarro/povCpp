#ifndef __I_TOKEN_STREAM_H__
#define __I_TOKEN_STREAM_H__

#include "io/pov/lexer/ReservedWord.h"
#include "io/pov/lexer/TokenStruct.h"

class ITokenStream {
  public:
    virtual ~ITokenStream() {}
    virtual ReservedWord *reservedWords() = 0;
    virtual TokenStruct &token() = 0;
    virtual void getToken() = 0;
    virtual void ungetToken() = 0;
    virtual bool canRewind() const { return false; }
    virtual int mark() { return -1; }
    virtual bool rewind(int) { return false; }
};

#endif
